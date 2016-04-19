#include "stdafx.h"
#include "config.h"
#include "Win32_Utils.h"
#include "CIOBuffer.h"
#include "ComHelper.h"
#include "FilterGuids.h"
#include "Uploader.h"

#define WAITFOR_INIT_TIMEOUT 10000

[ module(dll, name = "XiaozaiPushFilter", helpstring = "XiaozaiPushFilter 1.0 Type Library") ];
[ emitidl ];

// Setup data

const AMOVIESETUP_MEDIATYPE sudVideoPinTypes[] = 
{
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RGB24 },
    { &MEDIATYPE_Video, &WMMEDIASUBTYPE_MP4S },
    { &MEDIATYPE_Video, &WMMEDIASUBTYPE_WMV3 },
    { &MEDIATYPE_Video, &WMMEDIASUBTYPE_WMV2 },
    { &MEDIATYPE_Video, &WMMEDIASUBTYPE_WMV1 }
};

const AMOVIESETUP_MEDIATYPE sudAudioPinTypes[] = 
{
	{ &MEDIATYPE_Audio, &MEDIASUBTYPE_PCM  },
	{ &MEDIATYPE_Audio, &MEDIASUBTYPE_WAVE },
    { &MEDIATYPE_Audio, &WMMEDIASUBTYPE_WMAudioV8},
    { &MEDIATYPE_Audio, &WMMEDIASUBTYPE_WMAudioV9},
    { &MEDIATYPE_Audio, &WMMEDIASUBTYPE_MP3}
};

const AMOVIESETUP_PIN sudPins[] =
{
	{
		L"Video Input",						// Pin string name
			FALSE,								// Is it rendered
			FALSE,								// Is it an output
			FALSE,								// Allowed none
			TRUE,								// Likewise many
			&CLSID_NULL,						// Connects to filter(Obsolete.)
			L"Output",							// Connects to pin(Obsolete.)
			2,									// Number of types
			sudVideoPinTypes				    // Pin information
	},
	{
		L"Audio Input",						// Pin string name
			FALSE,								// Is it rendered
			FALSE,								// Is it an output
			FALSE,								// Allowed none
			TRUE,								// Likewise many
			&CLSID_NULL,						// Connects to filter(Obsolete.)
			L"Output",							// Connects to pin(Obsolete.)
			2,									// Number of types
			sudAudioPinTypes				    // Pin information
		}
};

const AMOVIESETUP_FILTER sudSUPeR =
{
	&CLSID_XiaozaiFilter,						// Filter CLSID
	L"Xiaozai Push Filter",// String name
	MERIT_DO_NOT_USE,					    // Filter merit
	2,									// Number pins
	sudPins								// Pin details
};


//
//  Object creation stuff
//
CFactoryTemplate g_Templates[]= {
    {
        L"Xiaozai Push Filter",
        &CLSID_XiaozaiFilter,
        CXiaozaiPushFilterImp::CreateInstance,
        NULL,
        &sudSUPeR
    },
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);    

CIOBuffer::Allocator g_MediaBufAllocator(100*1024,100);

IIOBuffer * SvsGetMediaBuffer(bool growForward = true, bool addRef = false)
{
	IIOBuffer * mybuf = g_MediaBufAllocator.Allocate(growForward,addRef);
	return mybuf;
};

////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

//
// DllRegisterSever
//
// Handle the registration of this filter
//
STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer


//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
					  DWORD  dwReason, 
					  LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

//------------------------------------------------------------------------------
// Name: wmCopyMediaType()
// Desc: Allocates memory for a WM_MEDIA_TYPE and its format data and 
//       copies an existing media type into it.
//------------------------------------------------------------------------------
HRESULT wmCopyMediaType(WM_MEDIA_TYPE** ppmtDest, WM_MEDIA_TYPE* pmtSrc)
{
	if (!ppmtDest)
	{
		return E_POINTER;
	}
	if (!pmtSrc)
	{
		return E_NOTIMPL;
	}

	// Create enough space for the media type and its format data
	*ppmtDest = (WM_MEDIA_TYPE*) new BYTE[sizeof(WM_MEDIA_TYPE) + pmtSrc->cbFormat];
	if (!*ppmtDest)
	{
		return E_OUTOFMEMORY;
	}

	// Copy the media type and the format data
	memcpy(*ppmtDest, pmtSrc, sizeof(WM_MEDIA_TYPE));
	// Format data is immediately after media type
	(*ppmtDest)->pbFormat = (((BYTE*) *ppmtDest) + sizeof(WM_MEDIA_TYPE)); 
	memcpy((*ppmtDest)->pbFormat, pmtSrc->pbFormat, pmtSrc->cbFormat);
	return S_OK;
}

// Constructor

CXiaozaiPushFilter::CXiaozaiPushFilter(CXiaozaiPushFilterImp *pSUPeR,
						   LPUNKNOWN pUnk,
						   CCritSec *pLock,
						   HRESULT *phr) :
CBaseFilter(NAME("CXiaozaiPushFilter"), pUnk, pLock, CLSID_XiaozaiFilter, phr),
m_pSUPeR(pSUPeR)
{
}


//
// GetPin
//
CBasePin * CXiaozaiPushFilter::GetPin(int n)
{
	switch (n)
	{
		case 0:
			return m_pSUPeR->m_pVideoInputPin;
		case 1:
			return m_pSUPeR->m_pAudioInputPin;
		case 2:
			return m_pSUPeR->m_pVideoOutputPin;
		case 3:
			return m_pSUPeR->m_pAudioOutputPin;
	default:
		break;
	}
	return NULL;
}


//
// GetPinCount
//
int CXiaozaiPushFilter::GetPinCount()
{
	return 4;
}


//
// Stop
//
// Overriden to close the SUPeR file
//
STDMETHODIMP CXiaozaiPushFilter::Stop()
{
	CAutoLock cObjectLock(m_pLock);

	DbgLog((LOG_TRACE,0,TEXT("Before StopUploading: %ul"), GetTickCount()));
	HRESULT hr = this->m_pSUPeR->StopUploading();
	RETURN_IF_FAILED(hr);

	DbgLog((LOG_TRACE,0,TEXT("Before CBaseFilter::Stop: %ul"), GetTickCount()));
	return CBaseFilter::Stop();
}


//
// Pause
//
// Overriden to open the SUPeR file
//
STDMETHODIMP CXiaozaiPushFilter::Pause()
{
	CAutoLock cObjectLock(m_pLock);

	ResetEvent(m_pSUPeR->m_hEvent);

	return CBaseFilter::Pause();
}


//
// Run
//
// Overriden to open the SUPeR file
//
STDMETHODIMP CXiaozaiPushFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cObjectLock(m_pLock);

	HRESULT hr = this->m_pSUPeR->StartUploading();
	RETURN_IF_FAILED(hr);

	return CBaseFilter::Run(tStart);
}


#define VIDEO_BITRATE 2*256*1024
#define AUDIO_BITRATE 4*32*1024

//
//  CXiaozaiPushFilterImp class
//
CXiaozaiPushFilterImp::CXiaozaiPushFilterImp(LPUNKNOWN pUnk, HRESULT *phr) :
CUnknown(NAME("CXiaozaiPushFilterImp"), pUnk),
m_pFilter(NULL),
m_pVideoInputPin(NULL),
m_pAudioInputPin(NULL),
m_pPosition(NULL),
//video parameters
m_msVideoBufferWindow(3000),
m_dwVideoBitrate(VIDEO_BITRATE),
m_rtVideoTime(0),
//audio parameters
m_msAudioBufferWindow(3000),
m_dwAudioBitrate(AUDIO_BITRATE),
m_rtAudioTime(0),
//others
m_pRawSnapshot(NULL),
m_rtLastSnapshot(0),
m_dwSnapshotFrequency(-1),
m_dwSnapshotLen(0),
m_bInit(FALSE),
m_bRunning(FALSE),
m_hEvent(INVALID_HANDLE_VALUE)
{
	ASSERT(phr);

	m_pFilter = new CXiaozaiPushFilter(this, GetOwner(), &m_Lock, phr);
	if (m_pFilter == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
		return;
	}

	m_pVideoInputPin = new CXiaozaiPushFilterVideoInputPin(this,GetOwner(),
		m_pFilter,
		&m_Lock,
		&m_ReceiveLock,
		phr,
		L"Video In"
		);
	m_pAudioInputPin = new CXiaozaiPushFilterAudioInputPin(this,GetOwner(),
		m_pFilter,
		&m_Lock,
		&m_ReceiveLock,
		phr,
		L"Audio In"
		);

	m_pAudioOutputPin = new CXiaozaiPushFilterAudioOutputPin(this, 
		m_pFilter,
		&m_Lock,
		phr,
		L"Audio Out"
		);

	m_pVideoOutputPin = new CXiaozaiPushFilterVideoOutputPin(this,
		m_pFilter,
		&m_Lock,
		phr,
		L"Video Out"
		);

	if (m_pVideoInputPin == NULL || m_pAudioInputPin == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
		return;
	}

	m_FFMpegAVControler = new XiaozaiAVController(GetOwner(), phr);

	m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

// Destructor

CXiaozaiPushFilterImp::~CXiaozaiPushFilterImp()
{
	SAFE_CLOSEHANDLE(m_hEvent);
	
    SAFE_DELETE(m_pVideoInputPin);
	SAFE_DELETE(m_pAudioInputPin);
	SAFE_DELETE(m_pAudioOutputPin);
	SAFE_DELETE(m_pVideoInputPin);
	SAFE_DELETE(m_pFilter);
	SAFE_DELETE(m_pPosition);
	SAFE_DELETE(m_FFMpegAVControler);
	SAFE_ARRAYDELETE(m_pRawSnapshot);
}


//
// CreateInstance
//
// Provide the way for COM to create a SUPeR filter
//
CUnknown * WINAPI CXiaozaiPushFilterImp::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	ASSERT(phr);

	CXiaozaiPushFilterImp *pNewObject = new CXiaozaiPushFilterImp(punk, phr);
	if (pNewObject == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
	}

	return pNewObject;

} // CreateInstance


//
// NonDelegatingQueryInterface
//
// Override this to say what interfaces we support where
//
STDMETHODIMP CXiaozaiPushFilterImp::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
	CheckPointer(ppv,E_POINTER);
	CAutoLock lock(&m_Lock);

	// Do we have this interface

	if (riid == IID_IBaseFilter || riid == IID_IMediaFilter || riid == IID_IPersist) {
		return m_pFilter->NonDelegatingQueryInterface(riid, ppv);
	} 
    
	else if (riid == IID_ISuperSettings) {
		return m_FFMpegAVControler->NonDelegatingQueryInterface(riid, ppv);
	}
	else if (riid == IID_ISnapshotProvider) {
		*ppv = static_cast<ISnapshotProvider *>(this);
		this->AddRef();
		return S_OK;
	}
	else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
		if (m_pPosition == NULL) 
		{

			HRESULT hr = S_OK;
			m_pPosition = new CPosPassThru(NAME("SUPeR Pass Through"),
				(IUnknown *) GetOwner(),
				(HRESULT *) &hr, m_pFilter->GetPin(0));
			if (m_pPosition == NULL) 
				return E_OUTOFMEMORY;

			if (FAILED(hr)) 
			{
				delete m_pPosition;
				m_pPosition = NULL;
				return hr;
			}
		}

		return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
	} 

	return CUnknown::NonDelegatingQueryInterface(riid, ppv);

} // NonDelegatingQueryInterface


//
//
// Write
//
// Write raw data to the file
//
HRESULT CXiaozaiPushFilterImp::Write(IMediaSample *pSample, BOOL bVideo)
{
	HRESULT hr = NOERROR;
    DWORD dwLen = 0;
	do 
	{
		REFERENCE_TIME rtStart, rtEnd;
		hr = pSample->GetTime(&rtStart, &rtEnd);
        if (hr == VFW_E_SAMPLE_TIME_NOT_SET)
        {
            hr = pSample->GetMediaTime(&rtStart, &rtEnd);
        }
		BREAK_IF_FAILED(hr);

		//CAutoLock lockit(&mSyncWriting);
		if(bVideo && !IsCompressedData(0))
		{
			m_rtVideoTime = rtStart;
			if(m_dwSnapshotFrequency >=0 && m_rtVideoTime < m_rtLastSnapshot + m_dwSnapshotFrequency)
			{
				if(m_pRawSnapshot == NULL)
				{
					return E_FAIL;
				}

				CAutoLock lock(&m_csSnapshotLock);

				BYTE *pData = NULL;
				hr = pSample->GetPointer(&pData);
				dwLen = pSample->GetActualDataLength();
                if (m_dwSnapshotLen >= dwLen)
                {
				    CheckPointer(pData,E_POINTER);
				    if(FAILED(hr) || IsBadReadPtr(pData,sizeof(BYTE)*m_dwSnapshotLen))
				    {
					    hr = E_POINTER;
					    return hr;
				    }
				    memcpy(m_pRawSnapshot,pData,m_dwSnapshotLen);
				    m_rtLastSnapshot = m_rtVideoTime;
				    DbgLog((LOG_TRACE,0,TEXT("============ capture a snapshot ===========")));
                }
			}
			DbgLog((LOG_TRACE,3,TEXT("---Uncompressed Video sample arrived, time: %I64i"), m_rtVideoTime));
		}
		else  if (!bVideo)
		{
			m_rtAudioTime = rtStart;
			DbgLog((LOG_TRACE,3,TEXT("---Audio sample arrived, time: %I64i"), m_rtAudioTime));
		}
        else
        {
            m_rtVideoTime = rtStart;
            DbgLog((LOG_TRACE,3,TEXT("---Compressed Video sample arrived, time: %I64i"), m_rtVideoTime));
        }

		QWORD duration = QWORD(rtEnd - rtStart);
		LONG dataSize  = pSample->GetActualDataLength();
		INSSBuffer * pWmSample = NULL;

		// add by Chundong WANG @ 2006-9-20
		// modified to use event handler
		if(!m_bRunning)
		{
			DbgLog((LOG_TRACE,0,TEXT("######  Waiting for init...")));
			DWORD result = WaitForSingleObject(m_hEvent, WAITFOR_INIT_TIMEOUT);//INFINITE);
			if(result == WAIT_TIMEOUT || !m_bRunning)
			{
				return S_FALSE;
			}

			//if(!m_pWriter)
			//	return E_UNEXPECTED;
		}

		CReadWriteSection::AutoReadLock mylock(m_csReadWrite);
		//if (m_pWriter == NULL)
		//	return E_UNEXPECTED;
		//hr = m_pWriter->AllocateSample(dataSize, &pWmSample);
		//if(FAILED(hr))
		//{
		//	do 
		//	{
		//		Sleep(3);
		//		// while rs is unavailable, this writer object will be released
		//		// but the first content sample will arrive anyway
		//		// so this detect is for it!
		//		if(m_pWriter==NULL)
		//			return VFW_E_RUNTIME_ERROR;
		//		hr = m_pWriter->AllocateSample(dataSize, &pWmSample);
		//	} while(FAILED(hr));
		//}
		BREAK_IF_FAILED(hr);
		if (SUCCEEDED(hr))
		{
			PBYTE pSrc;
			hr  = pSample->GetPointer(&pSrc);
			
		}
	} while(false);

	if(FAILED(hr))
	{
		if(hr == NS_E_TOO_MUCH_DATA)
		{
			DbgLog((LOG_TRACE,0,TEXT("Error: NS_E_TOO_MUCH_DATA")));
		}
		else if(hr == E_INVALIDARG)
		{
			DbgLog((LOG_TRACE,0,TEXT("Error: E_INVALIDARG")));
		}
		else if(hr == E_UNEXPECTED)
		{
			DbgLog((LOG_TRACE,0,TEXT("Error: E_UNEXPECTED")));
		}
		else
		{
			DbgLog((LOG_TRACE,0,TEXT("Error: %x"), hr));
		}
	}
	return hr;
}

HRESULT CXiaozaiPushFilterImp::StartUploading()
{
	HRESULT hr = S_OK;
	
	RETURN_IF_FAILED(hr);
	do 
	{        		
		if(!m_bRunning)
		{
			//initialize the outpins
			WORD nChannels;
			DWORD nSamplesPersec;
			WORD wBitsPerSample;
			m_FFMpegAVControler->GetAudioInfo(&nChannels, &nSamplesPersec, &wBitsPerSample);
			((CXiaozaiPushFilterAudioOutputPin*)m_pAudioOutputPin)->SetAudioFormat(nChannels, nSamplesPersec, wBitsPerSample);
			LONG Width;
			LONG Height;
			DOUBLE fps;
			m_FFMpegAVControler->GetVideoInfo(&Width, &Height, &fps);
			((CXiaozaiPushFilterVideoOutputPin*)m_pVideoOutputPin)->SetYUVFormat(Width, Height, fps);

			m_bRunning = TRUE;
			SetEvent(m_hEvent);

			m_FFMpegAVControler->Start(); //Start the working thread
		}
	} while(false);

	return hr;
}


HRESULT CXiaozaiPushFilterImp::StopUploading()
{
	HRESULT hr = S_OK;

	DbgLog((LOG_TRACE,0,TEXT("Before do...while: %ul"), GetTickCount()));
	do 
	{
		if(m_bRunning)
		{
			m_bRunning = FALSE;
			SetEvent(m_hEvent);

			DbgLog((LOG_TRACE,0,TEXT("Before m_pWriter->EndWriting: %ul"), GetTickCount()));			
		}
	} while(false);

	DbgLog((LOG_TRACE,0,TEXT("Before UninitEncoder: %ul"), GetTickCount()));
	RETURN_IF_FAILED(hr);

	

	return hr;
}


BOOL CXiaozaiPushFilterImp::IsCompressedData(int pinnum)
{
	BOOL bCompressed = FALSE, bVCompressed = TRUE, bACompressed = TRUE;

    //Check video pin
    //if ((m_pVideoInputPin->IsConnected())&&(m_mtVideo.subtype != WMMEDIASUBTYPE_WMV3)&&(m_mtVideo.subtype != WMMEDIASUBTYPE_MP4S)
    //    &&(m_mtVideo.subtype != WMMEDIASUBTYPE_MPEG2_VIDEO)&&(m_mtVideo.subtype != WMMEDIASUBTYPE_WMV2)&&(m_mtVideo.subtype != WMMEDIASUBTYPE_WMV1))
    if ((m_mtVideo.subtype == MEDIASUBTYPE_RGB24)&&(m_pVideoInputPin->IsConnected()))
    {
        bVCompressed = FALSE;
    }

    //Check audio pin
    //if ((m_pAudioInputPin->IsConnected()) && (m_mtAudio.subtype != WMMEDIASUBTYPE_WMAudioV9)&&(m_mtAudio.subtype != WMMEDIASUBTYPE_WMAudioV8)&&(m_mtAudio.subtype != WMMEDIASUBTYPE_MP3))
    if ((m_mtAudio.subtype == MEDIASUBTYPE_PCM)&&(m_pAudioInputPin->IsConnected()))
    {
        bACompressed = FALSE;
    }

    switch (pinnum)
    {
    case -1:  //check both video and audio pins

		    bCompressed = bVCompressed && bACompressed;
        break;

    case 0:  //check video pin only
		    bCompressed = bVCompressed;
        break;

    case 1:  //check audio pin only
            bCompressed = bACompressed;
        break;
	case 2:  //check video outpin only
		bCompressed = FALSE;
		break;
	case 3:  //check audio outpin only
		bCompressed = FALSE;
		break;

    default:
        break;
    }
	return bCompressed;

}


HRESULT CXiaozaiPushFilterImp::FillMediaSample(IMediaSample* pSamp, bool bVideo)
{
	REFERENCE_TIME rtStart = 0L, rtEnd = 0L;
	BYTE *pData;
	long lDataLen,actsize;
	pSamp->GetPointer(&pData);
	lDataLen = pSamp->GetSize();
	//ToDo:///
	m_FFMpegAVControler->FillMediaSample(pData, lDataLen, bVideo, &rtStart, &rtEnd, &actsize);

	pSamp->SetTime(&rtStart, &rtEnd);
	pSamp->SetActualDataLength(actsize);
	return S_OK;
}

