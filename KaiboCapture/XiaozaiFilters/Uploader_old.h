#include <wmsdk.h>

#include "dvdmedia.h"

#include "ID.h"
#include "ISuperSettings.h"
#include "ISnapshotProvider.h"
#include "IRecordProvider.h"
#include "Thread.h"

class CFMPEGInputPinBase;
class CSUPeRVideoInputPin;
class CSUPeRAudioInputPin;
class CXiaoZaiPushFilter;

using namespace OS;

// Main filter object

class CXiaoZaiPushFilter : public ISnapshotProvider,
	public IRecordProvider, public CBaseFilter//,CBaseRenderer
{
	friend class CFMPEGInputPinBase;
	friend class CSUPeRVideoInputPin;
	friend class CSUPeRAudioInputPin;

	CFMPEGInputPinBase *m_pVideoPin;		// A simple rendered input pin for video
	CFMPEGInputPinBase *m_pAudioPin;		// A simple rendered input pin for audio

	CCritSec m_Lock;					// Main renderer critical section
	CCritSec m_ReceiveLock;				// Sub lock for received samples

	CPosPassThru *m_pPosition;			// Renderer position controls

	CMediaType m_mtVideo;				// Media type of video
	CMediaType m_mtAudio;				// Media type of audio

	IWMWriter * m_pWriter;				// Encoder writer object;
	IWMWriterAdvanced * m_pWriterAdv;   // Encoder writer advanced interface
#ifdef INCLUDE_NETSINK
	IWMWriterSink *m_pSink;				// Writer sink
#endif
	IWMProfile *m_pProfile;				// profile

	//P2P interface
	ID                              m_SessionID;

	// video media infos
	WM_MEDIA_TYPE m_mtVideoStream;		// media type of video stream, different from m_mtVideo
	// m_mtVideo is recorded by video input pin, described the RAW video data
	WORD m_wVideoStrmNum;				// default stream number of video stream
	DWORD m_dwVideoInputNum;			// input number of video stream
	DWORD m_dwVideoBitrate;				// default bit rate of video stream
	DWORD m_msVideoBufferWindow;		// default buffer window of video stream, in milliseconds
	REFERENCE_TIME m_rtVideoTime;		// Current stream time

	// audio media infos
	WM_MEDIA_TYPE m_mtAudioStream;		// media type of audio stream, different from m_mtAudio
	// m_mtAudio is recorded by audio input pin, described the RAW audio data
	WORD m_wAudioStrmNum;				// default stream number of audio stream
	DWORD m_dwAudioInputNum;			// input number of audio stream
	DWORD m_dwAudioBitrate;				// default bit rate of audio stream
	DWORD m_msAudioBufferWindow;		// default buffer window of audio stream, in milliseconds
	REFERENCE_TIME m_rtAudioTime;		// Current stream time

	BOOL m_bInit;						// TRUE if InitEncoder has been called successfully
	BOOL m_bRunning;					// TRUE if running
	HANDLE m_hEvent;					// event object to wait for init

	BYTE *m_pRawSnapshot;				// pointer to the snapshot cache
	DWORD m_dwSnapshotLen;				// length of current snapshot
	REFERENCE_TIME m_rtLastSnapshot;	// time of last snapshot
	DWORD m_dwSnapshotFrequency;		// frequency of snapshot
	CCritSec m_csSnapshotLock;			// lock of snapshot

	BSTR	m_recordPath;				//record path

	CReadWriteSection m_csReadWrite; //lock of m_pWriter

public:
	DECLARE_IUNKNOWN

    // Constructor
	CXiaoZaiPushFilter(LPUNKNOWN pUnk,
                CCritSec *pLock,
                HRESULT *phr);

    // Pin enumeration
    CBasePin * GetPin(int n);
    int GetPinCount();

    // Open and close the file as necessary
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP Pause();
    STDMETHODIMP Stop();
 
public:
	
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

	// IRecordProvider
	HRESULT STDMETHODCALLTYPE SetRecordPath(const BSTR outputPath);


    // IStreamInfo
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

private:

	//HRESULT InitAllinOne();

	//HRESULT UninitAllinOne();

	//HRESULT InitP2P();

	//HRESULT UninitP2P();

	// uninitial the encoder objects, profiles.
	HRESULT UninitEncoder();

	// initial the encoder objects, profiles.
	HRESULT InitEncoder();

    // Overridden to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

private:
	// create a empty profile with media type copied from input pins
	HRESULT CreateCustomProfile(IWMProfile ** pProfile);

	// load system profile
	HRESULT CreateSystemProfile(IWMProfile ** pProfile);

	// check the input numbers
	HRESULT IdentifyInputsByNumber(void);

	//
	HRESULT ConnectionStreamType(WCHAR* inConnectionName, GUID* outType);

	//
	BOOL IsCodecAvailable(IWMProfileManager *pProfMgr, GUID inMajortype, GUID inSubtype, BYTE ** outMt);

	// Helper method
	HRESULT GetMediaTypeDetails(IWMMediaProps * pProps, BYTE** outDetails);

	BOOL IsCompressedData(int pinnum = -1); //if pinum = -1, check all pins; pinnum = 0, check video pin; pinnum = 1, check video 
};

