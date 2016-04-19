#pragma once
#include "Uploader.h"

class CXiaozaiPushFilterImp;

class CFMPEGOutputPinBase :
	public CAMThread, public CBaseOutputPin
{
protected:
	CCritSec *m_pLock = NULL;
	BOOL m_bZeroMemory;
public:
	CFMPEGOutputPinBase(
		__in_opt LPCTSTR pObjectName,
		__in CBaseFilter *pFilter,
		__in CCritSec *pLock,
		__inout HRESULT *phr,
		__in_opt LPCWSTR pName);

	virtual ~CFMPEGOutputPinBase();

protected:
	// *
	// * Data Source
	// *
	// * The following three functions: FillBuffer, OnThreadCreate/Destroy, are
	// * called from within the ThreadProc. They are used in the creation of
	// * the media samples this pin will provide
	// *

	// Override this to provide the worker thread a means
	// of processing a buffer
	virtual HRESULT FillBuffer(IMediaSample *pSamp) {
		BYTE *pData;
		long lDataLen;
		pSamp->GetPointer(&pData);
		lDataLen = pSamp->GetSize();

		if (m_bZeroMemory)
		{
			ZeroMemory(pData, lDataLen);
			m_bZeroMemory = FALSE;
		}
		return S_OK;
	};

	// Called as the thread is created/destroyed - use to perform
	// jobs such as start/stop streaming mode
	// If OnThreadCreate returns an error the thread will exit.
	virtual HRESULT OnThreadCreate(void) { m_bZeroMemory = TRUE;  return NOERROR; };
	virtual HRESULT OnThreadDestroy(void) { return NOERROR; };
	virtual HRESULT OnThreadStartPlay(void) { return NOERROR; };

	// *
	// * Worker Thread
	// *

	HRESULT Active(void);    // Starts up the worker thread
	HRESULT Inactive(void);  // Exits the worker thread.

public:
	// thread commands
	enum Command { CMD_INIT, CMD_PAUSE, CMD_RUN, CMD_STOP, CMD_EXIT };
	HRESULT Init(void) { return CallWorker(CMD_INIT); }
	HRESULT Exit(void) { return CallWorker(CMD_EXIT); }
	HRESULT Run(void) { return CallWorker(CMD_RUN); }
	HRESULT Pause(void) { return CallWorker(CMD_PAUSE); }
	HRESULT Stop(void) { return CallWorker(CMD_STOP); }
protected:
	Command GetRequest(void) { return (Command)CAMThread::GetRequest(); }
	BOOL    CheckRequest(Command *pCom) { return CAMThread::CheckRequest((DWORD *)pCom); }

	// override these if you want to add thread commands
	virtual DWORD ThreadProc(void);  		// the thread function

	virtual HRESULT DoBufferProcessingLoop(void);    // the loop executed whilst running


	// *
	// * AM_MEDIA_TYPE support
	// *

	// If you support more than one media type then override these 2 functions
	virtual HRESULT CheckMediaType(const CMediaType *pMediaType);
	virtual HRESULT GetMediaType(int iPosition, __inout CMediaType *pMediaType);  // List pos. 0-n

	// If you support only one type then override this fn.
	// This will only be called by the default implementations
	// of CheckMediaType and GetMediaType(int, CMediaType*)
	// You must override this fn. or the above 2!
	virtual HRESULT GetMediaType(__inout CMediaType *pMediaType) { return E_UNEXPECTED; }

	STDMETHODIMP QueryId(
		__deref_out LPWSTR * Id
		);
};

class CXiaozaiPushFilterAudioOutputPin : public CFMPEGOutputPinBase
{
	CXiaozaiPushFilterImp *m_pSUPeR = NULL;
	long m_nChannels;
	long m_nSamplePersec;
	long m_wBitsPerSample;
public:
	CXiaozaiPushFilterAudioOutputPin(CXiaozaiPushFilterImp *pSUPeR,
		__in CBaseFilter *pFilter,
		__in CCritSec *pLock,
		__inout HRESULT *phr,
		__in_opt LPCWSTR pName);

protected:
	// override this to set the buffer size and count. Return an error
	// if the size/count is not to your liking.
	// The allocator properties passed in are those requested by the
	// input pin - use eg the alignment and prefix members if you have
	// no preference on these.
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);

	// Check if the pin can support this specific proposed type and format
	virtual HRESULT GetMediaType(__inout CMediaType *pMediaType);

	// Override this to provide the worker thread a means
	// of processing a buffer
	virtual HRESULT FillBuffer(IMediaSample *pSamp);

public:
	HRESULT SetAudioFormat(WORD nChannels, DWORD nSamplePersec, WORD wBitsPerSample);
};

class CXiaozaiPushFilterVideoOutputPin : public CFMPEGOutputPinBase
{
	CXiaozaiPushFilterImp *m_pSUPeR = NULL;
	int m_nWidth;
	int m_nHeight;
public:
	CXiaozaiPushFilterVideoOutputPin(CXiaozaiPushFilterImp *pSUPeR,
		__in CBaseFilter *pFilter,
		__in CCritSec *pLock,
		__inout HRESULT *phr,
		__in_opt LPCWSTR pName);

protected:
	// override this to set the buffer size and count. Return an error
	// if the size/count is not to your liking.
	// The allocator properties passed in are those requested by the
	// input pin - use eg the alignment and prefix members if you have
	// no preference on these.
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);

	// Check if the pin can support this specific proposed type and format
	virtual HRESULT GetMediaType(__inout CMediaType *pMediaType);

	// Override this to provide the worker thread a means
	// of processing a buffer
	virtual HRESULT FillBuffer(IMediaSample *pSamp);

public:
	HRESULT SetRGBFormat(WORD iBitDepth, long Width, long Height, double fps);
	HRESULT SetYUVFormat(long Width, long Height, double fps);
};