#pragma once
#include <wmsdk.h>
#include "dvdmedia.h"
#include "IRecordProvider.h"
#include "ISnapshotProvider.h"
#include "Thread.h"
#include "ID.h"
#include "XiaozaiPushFilterOutPins.h"
#include "UploaderInputPins.h"
#include "XiaozaiAVController.h"


class CFMPEGInputPinBase;
class CXiaozaiPushFilterVideoInputPin;
class CXiaozaiPushFilterAudioInputPin;
class CXiaozaiPushFilterImp;
class CXiaozaiPushFilter;




using namespace OS;

// Main filter object

class CXiaozaiPushFilter : public CBaseFilter //,CBaseRenderer
{
    CXiaozaiPushFilterImp * const m_pSUPeR;

public:

    // Constructor
    CXiaozaiPushFilter(CXiaozaiPushFilterImp *pSUPeR,
                LPUNKNOWN pUnk,
                CCritSec *pLock,
                HRESULT *phr);

    // Pin enumeration
    CBasePin * GetPin(int n);
    int GetPinCount();

    // Open and close the file as necessary
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP Pause();
    STDMETHODIMP Stop();

	//
	// FindPinNumber
	//
	// return the number of the pin with this IPin* or -1 if none
	int FindPinNumber(__in IPin *iPin) {
		int i;
		for (i = 0; i<4; ++i) {
			if ((IPin *)(GetPin(i)) == iPin) {
				return i;
			}
		}
		return -1;
	}
};


//  CXiaozaiPushFilterImp object which has filter and pin members

class CXiaozaiPushFilterImp : public CUnknown, public ISnapshotProvider
{
    friend class CXiaozaiPushFilter;
    friend class CFMPEGInputPinBase;
    friend class CXiaozaiPushFilterVideoInputPin;
    friend class CXiaozaiPushFilterAudioInputPin;
	friend class XiaozaiAVController;

    CXiaozaiPushFilter   *m_pFilter;			// Methods for filter interfaces

	XiaozaiAVController  *m_FFMpegAVControler;

	// Input pins for future usage.
	// We may not actually have inputpins as we obtain video data directly from FFMPEG API.
	//
    CFMPEGInputPinBase *m_pVideoInputPin;		// A simple rendered input pin for video
    CFMPEGInputPinBase *m_pAudioInputPin;		// A simple rendered input pin for audio

	//
	// Output pins
	//
	CFMPEGOutputPinBase *m_pVideoOutputPin;
	CFMPEGOutputPinBase *m_pAudioOutputPin;

    CCritSec m_Lock;					// Main renderer critical section
    CCritSec m_ReceiveLock;				// Sub lock for received samples

    CPosPassThru *m_pPosition;			// Renderer position controls

	
	//This is the media tupes of input pins.
	//For output pins, we directly create in the implementation of pins, because the media types of 
	//our output pins are pre-determined.
	CMediaType m_mtVideo;				// Media type of input video
	CMediaType m_mtAudio;				// Media type of input audio

	//Video parameters
	DWORD m_dwVideoBitrate;				// default bit rate of video stream
	DWORD m_msVideoBufferWindow;		// default buffer window of video stream, in milliseconds
	REFERENCE_TIME m_rtVideoTime;		// Current stream time
	

	//Audio parameters
	DWORD m_dwAudioBitrate;				// default bit rate of audio stream
	DWORD m_msAudioBufferWindow;		// default buffer window of audio stream, in milliseconds
	REFERENCE_TIME m_rtAudioTime;		// Current stream time
	

	BOOL m_bInit;						// TRUE if the filter has been initialized successfully
	BOOL m_bRunning;					// TRUE if running
	HANDLE m_hEvent;					// event object to wait for init

	//Snapshot related members
	BYTE *m_pRawSnapshot;				// pointer to the snapshot cache
	DWORD m_dwSnapshotLen;				// length of current snapshot
	REFERENCE_TIME m_rtLastSnapshot;	// time of last snapshot
	DWORD m_dwSnapshotFrequency;		// frequency of snapshot
	CCritSec m_csSnapshotLock;			// lock of snapshot

	

	CReadWriteSection m_csReadWrite;	//lock of m_pWriter

public:

    DECLARE_IUNKNOWN

    CXiaozaiPushFilterImp(LPUNKNOWN pUnk, HRESULT *phr);
    ~CXiaozaiPushFilterImp();

	
	//ISnapshotProvider
	HRESULT STDMETHODCALLTYPE SetRefresgFrequency(DWORD frequency)
	{
		m_dwSnapshotFrequency = frequency;
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetCurrentSnapshot(BYTE *ppData, DWORD *datalen)
	{
		if(ppData==NULL)
		{
			if(datalen==NULL || IsBadWritePtr(datalen,sizeof(DWORD)))
				return E_POINTER;
			*datalen = m_dwSnapshotLen;

			return S_OK;
		}

		if(ppData==NULL || IsBadWritePtr(ppData,sizeof(BYTE)*m_dwSnapshotLen)
			|| datalen==NULL || IsBadWritePtr(datalen,sizeof(DWORD)))
			return E_POINTER;

		CAutoLock lock(&m_csSnapshotLock);

		memcpy(ppData,m_pRawSnapshot,m_dwSnapshotLen);
		*datalen = m_dwSnapshotLen;

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetBMPSetting(LONG *width, LONG *height, WORD *bitcount)
	{
		if(width==NULL || IsBadWritePtr(width,sizeof(LONG))
			|| height==NULL || IsBadWritePtr(height,sizeof(LONG))
			|| bitcount==NULL || IsBadWritePtr(bitcount,sizeof(WORD))
			)
			return E_POINTER;

		if(m_mtVideo.formattype == FORMAT_VideoInfo)
		{
			VIDEOINFOHEADER *pvihRecord = 
				reinterpret_cast<VIDEOINFOHEADER *>(m_mtVideo.pbFormat);
			*width = pvihRecord->bmiHeader.biWidth;
			*height = pvihRecord->bmiHeader.biHeight;
			*bitcount = pvihRecord->bmiHeader.biBitCount;// must be 24
		}
		else if(m_mtVideo.formattype == FORMAT_VideoInfo2)
		{
			VIDEOINFOHEADER2 *pvihRecord = 
				reinterpret_cast<VIDEOINFOHEADER2 *>(m_mtVideo.pbFormat);
			*width = pvihRecord->bmiHeader.biWidth;
			*height = pvihRecord->bmiHeader.biHeight;
			*bitcount = pvihRecord->bmiHeader.biBitCount;// must be 24
		}

		return S_OK;
	}

    HRESULT STDMETHODCALLTYPE GetTimeStamp(REFERENCE_TIME* llTime)
    {
        CheckPointer(llTime,E_POINTER);
        *llTime = m_rtVideoTime;
        return S_OK;
    };

	
	

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    // Write raw data stream to a file
    HRESULT Write(IMediaSample *pSample, BOOL bVideo);

	// Start the write object.
	HRESULT StartUploading();

	// Stop the write object.
	HRESULT StopUploading();

	///
	///function for DShow interface
	///

	HRESULT FillMediaSample(IMediaSample* pSamp, bool bVideo);

    // Overridden to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

private:

	BOOL IsCompressedData(int pinnum = -1); //if pinum = -1, check all pins; pinnum = 0, check video pin; pinnum = 1, check video 
	

};