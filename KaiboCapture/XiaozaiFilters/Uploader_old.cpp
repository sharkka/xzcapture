#include "stdafx.h"
#include "config.h"

#include "Win32_Utils.h"

#include "CIOBuffer.h"
#include "ComHelper.h"
#include "Uploader.h"
#include "UploaderInputPins.h"
#include "DvdMedia.h"
#include "FilterGuids.h"

#define WAITFOR_INIT_TIMEOUT 10000

[ module(dll, name = "XiaozaiFilter", helpstring = "XiaozaiFilter 1.0 Type Library") ];
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
		CXiaoZaiPushFilter::CreateInstance,
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


//
// GetPin
//
CBasePin * CXiaoZaiPushFilter::GetPin(int n)
{
	if (n == 0) {
		return m_pVideoPin;
	} else if (n == 1) {
		return m_pAudioPin;
	} else {
		return NULL;
	}
}


//
// GetPinCount
//
int CXiaoZaiPushFilter::GetPinCount()
{
	return 2;
}


//
// Stop
//
// Overriden to close the SUPeR file
//
STDMETHODIMP CXiaoZaiPushFilter::Stop()
{
	CAutoLock cObjectLock(m_pLock);

	DbgLog((LOG_TRACE,0,TEXT("Before StopUploading: %ul"), GetTickCount()));
	HRESULT hr = this->StopUploading();
	RETURN_IF_FAILED(hr);

	DbgLog((LOG_TRACE,0,TEXT("Before CBaseFilter::Stop: %ul"), GetTickCount()));
	return CBaseFilter::Stop();
}


//
// Pause
//
// Overriden to open the SUPeR file
//
STDMETHODIMP CXiaoZaiPushFilter::Pause()
{
	CAutoLock cObjectLock(m_pLock);

	ResetEvent(m_hEvent);

	return CBaseFilter::Pause();
}


//
// Run
//
// Overriden to open the SUPeR file
//
STDMETHODIMP CXiaoZaiPushFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cObjectLock(m_pLock);

	HRESULT hr = this->StartUploading();
	RETURN_IF_FAILED(hr);

	return CBaseFilter::Run(tStart);
}


#define VIDEO_BITRATE 2*256*1024
#define AUDIO_BITRATE 4*32*1024

//
//  CXiaoZaiPushFilter class
//  Constructor
//
CXiaoZaiPushFilter::CXiaoZaiPushFilter(LPUNKNOWN pUnk,
	CCritSec *pLock,
	HRESULT *phr) :
	CBaseFilter(NAME("CXiaoZaiPushFilter"), pUnk, pLock, CLSID_XiaozaiFilter, phr),
m_pVideoPin(NULL),
m_pAudioPin(NULL),
m_pPosition(NULL),
m_pWriter(NULL),
m_pWriterAdv(NULL),
#ifdef INCLUDE_NETSINK
m_pSink(NULL),
#endif
m_pProfile(NULL),

//video parameters
m_wVideoStrmNum(1),
m_dwVideoInputNum(0),
m_msVideoBufferWindow(3000),
m_dwVideoBitrate(VIDEO_BITRATE),
m_rtVideoTime(0),
//audio parameters
m_wAudioStrmNum(2),
m_dwAudioInputNum(0),
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

    m_SessionID.Initialize();

	m_pVideoPin = new CSUPeRVideoInputPin(this,GetOwner(),
		this,
		&m_Lock,
		&m_ReceiveLock,
		phr,
		L"Video In"
		);
	m_pAudioPin = new CSUPeRAudioInputPin(this,GetOwner(),
		this,
		&m_Lock,
		&m_ReceiveLock,
		phr,
		L"Audio In"
		);

	ZeroMemory(&m_mtVideoStream,sizeof(WM_MEDIA_TYPE));
	ZeroMemory(&m_mtAudioStream,sizeof(WM_MEDIA_TYPE));

	if (m_pVideoPin == NULL || m_pAudioPin == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
		return;
	}

	m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

// Destructor

CXiaoZaiPushFilter::~CXiaoZaiPushFilter()
{
	SAFE_CLOSEHANDLE(m_hEvent);
    
    //SAFE_RELEASE(m_pUploadingPeer);
	
    SAFE_DELETE(m_pVideoPin);
	SAFE_DELETE(m_pAudioPin);
	SAFE_DELETE(m_pPosition);
	SAFE_ARRAYDELETE(m_pRawSnapshot);
}


//
// CreateInstance
//
// Provide the way for COM to create a SUPeR filter
//
CUnknown * WINAPI CXiaoZaiPushFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	ASSERT(phr);

	CXiaoZaiPushFilter *pNewObject = new CXiaoZaiPushFilter(punk, phr);
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
STDMETHODIMP CXiaoZaiPushFilter::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
	CheckPointer(ppv,E_POINTER);
	CAutoLock lock(&m_Lock);

	// Do we have this interface

	if (riid == IID_IBaseFilter || riid == IID_IMediaFilter || riid == IID_IPersist) {
		return this->NonDelegatingQueryInterface(riid, ppv);
	} 
    /*
	else if (riid == IID_ISuperSettings) {
		*ppv = static_cast<ISuperSettings *>(this);
		this->AddRef();
		return S_OK;
	}*/
	else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
		if (m_pPosition == NULL) 
		{

			HRESULT hr = S_OK;
			m_pPosition = new CPosPassThru(NAME("SUPeR Pass Through"),
				(IUnknown *) GetOwner(),
				(HRESULT *) &hr, this->GetPin(0));
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

// Distinguish which input is audio and which input is video
HRESULT CXiaoZaiPushFilter::IdentifyInputsByNumber(void)
{
	HRESULT hr = S_OK;
	// Get the total number of inputs for the file.
	DWORD cInputs  = 0;
	hr = m_pWriter->GetInputCount(&cInputs);
	if (FAILED(hr))
	{
		return hr;
	}

	WCHAR*   pwszName = NULL;
	WORD     cchName  = 0;
	IWMInputMediaProps* pProps = NULL;

	// Loop through all supported inputs.
	for (DWORD inputIndex = 0; inputIndex < cInputs; inputIndex++)
	{
		// Get the input properties for the input.
		hr = m_pWriter->GetInputProps(inputIndex, &pProps);  
		BREAK_IF_FAILED(hr);

		// Get the size of the connection name.
		hr = pProps->GetConnectionName(0, &cchName);
		BREAK_IF_FAILED(hr);

		if (cchName > 0)
		{
			// Allocate memory for the connection name.
			pwszName = new WCHAR[cchName];
			if (pwszName == NULL)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			// Get the connection name.
			hr = pProps->GetConnectionName(pwszName, &cchName);
			BREAK_IF_FAILED(hr);

			DbgLog((LOG_TRACE,0,TEXT("Input # %d = %S"),inputIndex, pwszName));			
			GUID guidType;
			hr = ConnectionStreamType(pwszName, &guidType);
			BREAK_IF_FAILED(hr);

			// Save this input number!
			if (guidType == WMMEDIATYPE_Audio)
			{
				m_dwAudioInputNum = inputIndex;
			}
			else if (guidType == WMMEDIATYPE_Video)
			{
				m_dwVideoInputNum = inputIndex;
			}			
		} 

		// Clean up for next iteration.
		SAFE_ARRAYDELETE(pwszName);
		SAFE_RELEASE(pProps);
	}

	SAFE_ARRAYDELETE(pwszName);
	SAFE_RELEASE(pProps);

	return hr;
}

//
HRESULT CXiaoZaiPushFilter::ConnectionStreamType(WCHAR * inConnectionName, GUID* outType)
{
	if (!m_pProfile)
	{
		return E_FAIL;
	}

	// Get the total stream count defined in the profile
	HRESULT hr = S_OK;
	DWORD streamCount = 0;	
	hr = m_pProfile->GetStreamCount(&streamCount);
	if (FAILED(hr))
	{
		return hr;
	}

	WORD  streamNum = 0;
	WCHAR* pwszName = NULL;
	WORD   cchName  = 0;
	IWMStreamConfig *pConfig = NULL;

	// Loop through every stream
	for (DWORD dwIndex = 0; dwIndex < streamCount; dwIndex++)
	{
		// Get the stream config
		hr = m_pProfile->GetStream(dwIndex, &pConfig);
		BREAK_IF_FAILED(hr);

		// Get the connection name's size
		hr = pConfig->GetConnectionName(0, &cchName);
		BREAK_IF_FAILED(hr);

		if (cchName > 0)
		{
			// Allocate memory for the connection name.
			pwszName = new WCHAR[cchName];
			if (pwszName == NULL)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			// Get the connection name
			hr = pConfig->GetConnectionName(pwszName, &cchName);
			BREAK_IF_FAILED(hr);

			// Compare the two string
			if (wcscmp(pwszName, inConnectionName) == 0)
			{
				hr = pConfig->GetStreamType(outType);
				// Since we find the connection, terminate the iteration
				break;
			}
		}

		SAFE_ARRAYDELETE(pwszName);
		SAFE_RELEASE(pConfig);
	}

	SAFE_ARRAYDELETE(pwszName);
	SAFE_RELEASE(pConfig);

	return hr;
}

//
// Write
//
// Write raw data to the file
//
HRESULT CXiaoZaiPushFilter::Write(IMediaSample *pSample, BOOL bVideo)
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

			if(!m_pWriter)
				return E_UNEXPECTED;
		}

		CReadWriteSection::AutoReadLock mylock(m_csReadWrite);
		if (m_pWriter == NULL)
			return E_UNEXPECTED;
		hr = m_pWriter->AllocateSample(dataSize, &pWmSample);
		if(FAILED(hr))
		{
			do 
			{
				Sleep(3);
				// while rs is unavailable, this writer object will be released
				// but the first content sample will arrive anyway
				// so this detect is for it!
				if(m_pWriter==NULL)
					return VFW_E_RUNTIME_ERROR;
				hr = m_pWriter->AllocateSample(dataSize, &pWmSample);
			} while(FAILED(hr));
		}
		BREAK_IF_FAILED(hr);
		if (SUCCEEDED(hr))
		{
			PBYTE pSrc, pDest;
			hr  = pSample->GetPointer(&pSrc);
			hr |= pWmSample->GetBuffer(&pDest);
			memcpy(pDest, pSrc, dataSize);
			hr |= pWmSample->SetLength(dataSize);
			if(FAILED(hr))
				break;
			BREAK_IF_FAILED(hr);

			// Send sample to the writer
			DWORD flags = 0;
			if (S_OK == pSample->IsSyncPoint())
			{
				flags |= WM_SF_CLEANPOINT;
			}
			if (S_OK == pSample->IsDiscontinuity())
			{
				flags |= WM_SF_DISCONTINUITY;
			}

			if(bVideo)
			{
				if(!IsCompressedData(0))
				{
					hr = m_pWriter->WriteSample(m_dwVideoInputNum, rtStart, flags, pWmSample);
					if(FAILED(hr))
					{
						hr = m_pWriter->WriteSample(m_dwVideoInputNum, rtStart, flags, pWmSample);
						BREAK_IF_FAILED(hr);
					}
					BREAK_IF_FAILED(hr);
				}
				else
				{
					

					hr = m_pWriterAdv->WriteStreamSample(m_wVideoStrmNum, rtStart, 0, 0, flags, pWmSample);
					if(FAILED(hr))
					{
						hr = m_pWriterAdv->WriteStreamSample(m_wVideoStrmNum, rtStart, 0, 0, flags, pWmSample);
						BREAK_IF_FAILED(hr);
					}
					BREAK_IF_FAILED(hr);
				}
			}
			else
			{
                if(!IsCompressedData(1))
				{
				    hr = m_pWriter->WriteSample(m_dwAudioInputNum, rtStart, flags, pWmSample);
				    if(FAILED(hr))
				    {
					    hr = m_pWriter->WriteSample(m_dwAudioInputNum, rtStart, flags, pWmSample);
					    if(FAILED(hr))
						    break;
				    }
				    BREAK_IF_FAILED(hr);
                }
                else
                {
                    hr = m_pWriterAdv->WriteStreamSample(m_wAudioStrmNum, rtStart, 0, 0, flags, pWmSample);
					if(FAILED(hr))
					{
						hr = m_pWriterAdv->WriteStreamSample(m_wAudioStrmNum, rtStart, 0, 0, flags, pWmSample);
						BREAK_IF_FAILED(hr);
					}
					BREAK_IF_FAILED(hr);
                }
			}
			pWmSample->Release();
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

HRESULT CXiaoZaiPushFilter::StartUploading()
{
	HRESULT hr = S_OK;

    SetRecordPath(L"d:/e.asf");

	hr = InitEncoder();
	
	RETURN_IF_FAILED(hr);

	do 
	{
        //m_pUploadingPeer->CreateUploadSession(m_SessionID, L"Super.pmpx", UNSPECIFIED_OUTDEGREE);
        		
		if(!m_bRunning)
		{
			hr = m_pWriter->BeginWriting();
			BREAK_IF_FAILED(hr);

			m_bRunning = TRUE;
			SetEvent(m_hEvent);
		}
	} while(false);

	if(FAILED(hr))
	{
       // m_pUploadingPeer->DestroyUploadSession(m_SessionID);
	}
	return hr;
}


HRESULT CXiaoZaiPushFilter::StopUploading()
{
	HRESULT hr = S_OK;

	DbgLog((LOG_TRACE,0,TEXT("Before do...while: %ul"), GetTickCount()));
	do 
	{
        //m_pUploadingPeer->DestroyUploadSession(m_SessionID);

		if(m_bRunning)
		{
			m_bRunning = FALSE;
			SetEvent(m_hEvent);

			DbgLog((LOG_TRACE,0,TEXT("Before m_pWriter->EndWriting: %ul"), GetTickCount()));			
		}
	} while(false);

	DbgLog((LOG_TRACE,0,TEXT("Before UninitEncoder: %ul"), GetTickCount()));
	hr = UninitEncoder();
	RETURN_IF_FAILED(hr);

	return hr;
}

HRESULT CXiaoZaiPushFilter::UninitEncoder()
{
	HRESULT hr = S_OK;
	IWMWriterNetworkSink *pNetSink = NULL;

	if(!m_bInit)
		return hr;

	do 
	{
		if(m_pWriter != NULL)
		{
			DbgLog((LOG_TRACE,0,TEXT("Before m_pWriterAdv->RemoveSink: %ul"), GetTickCount()));
			// remove all sinks
			hr = m_pWriterAdv->RemoveSink(NULL);
			BREAK_IF_FAILED(hr);
		}

#ifdef INCLUDE_NETSINK
		if(m_pSink != NULL)
		{
			// close the network sink
			hr = m_pSink->QueryInterface(IID_IWMWriterNetworkSink, reinterpret_cast<void **>(&pNetSink));
			BREAK_IF_FAILED(hr);

			DbgLog((LOG_TRACE,0,TEXT("Before pNetSink->Close: %ul"), GetTickCount()));
			hr = pNetSink->Close();
			BREAK_IF_FAILED(hr);
			SAFE_RELEASE(pNetSink);
            SAFE_RELEASE(m_pSink);
		}

#endif
		//if(m_pUploadingPeer!=NULL)
	
  //      {
		//	// remove m_pPPStreamer from m_pPacketization
		//	DbgLog((LOG_TRACE,0,TEXT("Before UnRegPackSink and UnRegPackEventHandler: %ul"), GetTickCount()));
		//	
  //          m_pUploadingPeer->DestroyUploadSession(m_SessionID);
		//	
		//}
		// release it again
		//SAFE_RELEASE(m_pUploadingPeer);		
		SAFE_RELEASE(m_pWriterAdv);
		SAFE_RELEASE(m_pProfile);
		{
			CReadWriteSection::AutoWriteLock mylock(m_csReadWrite);
			SAFE_RELEASE(m_pWriter);
		}

		m_bInit = FALSE;

	} while(false);

	DbgLog((LOG_TRACE,0,TEXT("Before return from UninitEncoder: %ul"), GetTickCount()));
	return hr;
}

HRESULT CXiaoZaiPushFilter::InitEncoder()
{
	if (m_bInit)
	{
		return S_OK;
	}

	HRESULT hr = S_OK;
	IWMWriterNetworkSink *pNetSink = NULL;
	IWMWriterSink *pSvsSink = NULL;

	do 
	{
		CReadWriteSection::AutoWriteLock mylock(m_csReadWrite);
		hr = WMCreateWriter(NULL,&this->m_pWriter);
		BREAK_IF_FAILED(hr);


		hr = CreateCustomProfile(&m_pProfile);
		BREAK_IF_FAILED(hr);

		//hr = CreateSystemProfile(&m_pProfile);
		//BREAK_IF_FAILED(hr);

		// -----------------------------------------------------------------------------------
		// ----------------------- set profile to the writer ---------------------------------
		// -----------------------------------------------------------------------------------

		hr = m_pWriter->SetProfile(m_pProfile);
		BREAK_IF_FAILED(hr);

		hr = m_pWriter->QueryInterface(IID_IWMWriterAdvanced, reinterpret_cast<void **>(&m_pWriterAdv));
		BREAK_IF_FAILED(hr);

		hr = m_pWriterAdv->SetLiveSource(TRUE);
		BREAK_IF_FAILED(hr);

        hr = m_pWriterAdv->SetSyncTolerance(10000);
        BREAK_IF_FAILED(hr);

#ifdef INCLUDE_NETSINK
		hr = WMCreateWriterNetworkSink(&pNetSink);
		BREAK_IF_FAILED(hr);

		DWORD dwPort = 23472;
		hr = pNetSink->Open(&dwPort);
		BREAK_IF_FAILED(hr);

		hr = pNetSink->QueryInterface(IID_IWMWriterSink, reinterpret_cast<void **>(&m_pSink));
		BREAK_IF_FAILED(hr);

		hr = m_pWriterAdv->AddSink(m_pSink);
		BREAK_IF_FAILED(hr);
#endif

        /*CoCreateInstance(CLSID_UploadingPeer, 
                            NULL, 
                            CLSCTX_INPROC_SERVER, 
                            IID_IUploadingPeer, 
                            (void **) &m_pUploadingPeer);
		BREAK_IF_FAILED(hr);

        if (m_pUploadingPeer != NULL)
        {
            hr = m_pUploadingPeer->QueryWriterSink(m_SessionID, &pSvsSink);
            BREAK_IF_FAILED(hr);

		    hr = m_pWriterAdv->AddSink(pSvsSink);
            BREAK_IF_FAILED(hr);
        }*/

		//identify all input numbers of a/v
		hr = IdentifyInputsByNumber();

        if (m_pVideoPin->IsConnected() && IsCompressedData(0))
        {
            //Set video input propert interface 
            hr = m_pWriter->SetInputProps(m_dwVideoInputNum,NULL);
            BREAK_IF_FAILED(hr);
        }

        if (m_pAudioPin->IsConnected() && IsCompressedData(1))
        {
            //Set video input propert interface 
            hr = m_pWriter->SetInputProps(m_dwAudioInputNum,NULL);
            BREAK_IF_FAILED(hr);
        }

		m_bInit = TRUE;

	} while(false);

	SAFE_RELEASE(pNetSink);
	SAFE_RELEASE(pSvsSink);

	return hr;
}

// load system profile
HRESULT CXiaoZaiPushFilter::CreateSystemProfile(IWMProfile ** ppProfile)
{
	HRESULT hr = S_OK;
	IWMProfileManager *pProfileManager = NULL;
	IWMProfile *pProfile = NULL;

	do 
	{
		hr = WMCreateProfileManager(&pProfileManager);
		BREAK_IF_FAILED(hr);

		hr = pProfileManager->LoadProfileByID(WMProfile_V80_384PALVideo, &pProfile);
		BREAK_IF_FAILED(hr);
	} while(false);

	SAFE_RELEASE(pProfileManager);
	*ppProfile = pProfile;
	// no need to release pProfile here
	return hr;
}


// Helper method
HRESULT CXiaoZaiPushFilter::GetMediaTypeDetails(IWMMediaProps * pProps, BYTE** outDetails)
{
	if (pProps == NULL)
	{
		return E_FAIL;
	}

	HRESULT hr = NOERROR;
	BYTE * pMediaType = NULL;
	DWORD  cbTypeSize = 0;
	do
	{
		// Get the size of the media type structure.		
		hr = pProps->GetMediaType(NULL, &cbTypeSize);
		BREAK_IF_FAILED(hr);

		// Allocate memory for the media type structure.
		pMediaType = new BYTE[cbTypeSize];
		if (pMediaType == NULL)
		{
			hr = E_OUTOFMEMORY;
			break;
		}

		// Get the media type structure.
		hr = pProps->GetMediaType((WM_MEDIA_TYPE*)pMediaType, &cbTypeSize);

	} while (FALSE);

	if (SUCCEEDED(hr))
	{
		*outDetails = pMediaType;
	}
	else
	{
		if (pMediaType)
		{
			delete[] pMediaType;
			pMediaType = NULL;
		}
	}
	return hr;
}

// Check if specified codec exists. If yes, copy mediatype out.
// When invoking, *outMt must be NULL!
// A special case:
// If inMajortype is audio, *outMt must point to a WAVEFORMATEX providing 
// some desired formats, such as samplerate, channels, bits, bitrate.
BOOL CXiaoZaiPushFilter::IsCodecAvailable(IWMProfileManager *pProfMgr, GUID inMajortype, GUID inSubtype, BYTE ** outMt)
{
	HRESULT hr = NOERROR;
	BOOL found = FALSE;
	IWMCodecInfo * pCodecInfo = NULL;

	do
	{
		// Get the IWMCodecInfo interface
		hr = pProfMgr->QueryInterface(IID_IWMCodecInfo, (void**) &pCodecInfo);
		if (FAILED(hr))
		{
			break;
		}

		IWMStreamConfig*	pConfig = NULL;
		IWMMediaProps*		pProps  = NULL;
		BYTE *	pMediaType = NULL;

		// Retrieve the number of supported codecs on the system.
		DWORD cCodecs  = 0;
		hr = pCodecInfo->GetCodecInfoCount(inMajortype, &cCodecs);

		// Loop through all the codecs
		for (DWORD i = 0; i < cCodecs && !found; i++)
		{
			// Get the total count of supported stream formats
			DWORD cFormats   = 0;
			hr = pCodecInfo->GetCodecFormatCount(inMajortype, i, &cFormats);
			BREAK_IF_FAILED(hr);

			if (cFormats <= 0)
			{
				// Unexpected!
				continue;
			}

			// Get stream config interfaces (of the first supported format)
			hr = pCodecInfo->GetCodecFormat(inMajortype, i, 0, &pConfig);
			BREAK_IF_FAILED(hr);
			hr = pConfig->QueryInterface(IID_IWMMediaProps, (void**)&pProps);
			BREAK_IF_FAILED(hr);

			// Get the media type details
			hr = GetMediaTypeDetails(pProps, &pMediaType);
			BREAK_IF_FAILED(hr);

			// Check if the right codec?
			WM_MEDIA_TYPE * pwmType = (WM_MEDIA_TYPE*) pMediaType;
			if (pwmType->subtype == inSubtype)
			{
#ifdef _DEBUG
				WMVIDEOINFOHEADER * pvi = (WMVIDEOINFOHEADER*) pwmType->pbFormat;
				LONG formatSize = pwmType->cbFormat;
				LONG viSize = sizeof(WMVIDEOINFOHEADER);
				if (inMajortype == WMMEDIATYPE_Audio)
				{
					viSize = sizeof(WAVEFORMATEX);
				}

				if (formatSize > viSize)
				{
					int enter = 0;
					//	TRACE("Extra data is appended after normal block.\n");
				}
#endif // _DEBUG

				// !!! Special case for audio codec !!!
				if (inMajortype == WMMEDIATYPE_Audio)
				{
					// Check if outMt is valid?
					if (*outMt == NULL)
					{
						return E_INVALIDARG;
					}
					WAVEFORMATEX * pDesired = (WAVEFORMATEX *) (*outMt);

					HRESULT hr2 = NOERROR;
					IWMStreamConfig*	pConfig2 = NULL;
					IWMMediaProps*		pProps2  = NULL;
					BYTE *	pMediaType2 = NULL;

					DWORD dwBestRate = 0;
					DWORD dwMaxRate = this->m_dwAudioBitrate;

					// Enumerate all supported formats
					for (DWORD j = 0; j < cFormats; j++)
					{
						hr2 = pCodecInfo->GetCodecFormat(WMMEDIATYPE_Audio, i, j, &pConfig2);
						BREAK_IF_FAILED(hr2);
						hr2 = pConfig2->QueryInterface(IID_IWMMediaProps, (void**)&pProps2);
						BREAK_IF_FAILED(hr2);
						hr2 = GetMediaTypeDetails(pProps2, &pMediaType2);
						BREAK_IF_FAILED(hr2);

						// Check if the current stream format match our needs
						pwmType = (WM_MEDIA_TYPE*) pMediaType2;
						WAVEFORMATEX * pStream = (WAVEFORMATEX*) pwmType->pbFormat;
						if ( (pStream->nAvgBytesPerSec*8) > dwBestRate
							&& (pStream->nAvgBytesPerSec*8) <= dwMaxRate)
						{
							if (pStream->nSamplesPerSec == pDesired->nSamplesPerSec &&
								pStream->nChannels == pDesired->nChannels &&
								pStream->wBitsPerSample == pDesired->wBitsPerSample)/* &&
								pStream->nBlockAlign * pStream->nSamplesPerSec >= pDesired->nAvgBytesPerSec)*/
							{
								if((pStream->nAvgBytesPerSec / pStream->nBlockAlign) >= 
									((pStream->nAvgBytesPerSec >= 4000) ? 5.0 : 3.0))
								{
									// Save this media type for later use
									wmCopyMediaType((WM_MEDIA_TYPE**)outMt, pwmType);
									dwBestRate = pStream->nAvgBytesPerSec*8;
									//found = TRUE;
									//break;
								}
							}
						}

						SAFE_ARRAYDELETE(pMediaType2);
						SAFE_RELEASE(pProps2);
						SAFE_RELEASE(pConfig2);
					}// Enumerate all supported formats

					SAFE_ARRAYDELETE(pMediaType2);
					SAFE_RELEASE(pProps2);
					SAFE_RELEASE(pConfig2);

					if (dwBestRate > 0)
					{
						this->m_dwAudioBitrate = dwBestRate;
						found = TRUE;
						break;
					}
				}
				else
				{
					// Save this media type for later use
					wmCopyMediaType((WM_MEDIA_TYPE**)outMt, pwmType);
					found = TRUE;
					break;
				}
			}

			SAFE_ARRAYDELETE(pMediaType);
			SAFE_RELEASE(pProps);
			SAFE_RELEASE(pConfig);
		}// Loop through all the codecs

		SAFE_ARRAYDELETE(pMediaType);
		SAFE_RELEASE(pProps);
		SAFE_RELEASE(pConfig);

	} while (FALSE);

	SAFE_RELEASE(pCodecInfo);

	return found;
}

// create a empty profile with media type copied from input pins
HRESULT CXiaoZaiPushFilter::CreateCustomProfile(IWMProfile ** ppProfile)
{
	HRESULT hr = S_OK;
	IWMProfileManager *pProfileManager = NULL;
	IWMProfile *pProfile = NULL;
	IWMStreamConfig *pStrmCfg = NULL;
	IWMMediaProps *pMediaProp = NULL;
	WM_MEDIA_TYPE * pMediaType  = NULL;
	BYTE * pType = NULL;

	do 
	{
		hr = WMCreateProfileManager(&pProfileManager);
		BREAK_IF_FAILED(hr);

		hr = pProfileManager->CreateEmptyProfile(WMT_VER_9_0,&pProfile);
		BREAK_IF_FAILED(hr);

		// -----------------------------------------------------------------------------------
		// ------------------------- configure video stream ----------------------------------
		// -----------------------------------------------------------------------------------
		if(m_pVideoPin->IsConnected())
		{
			// create a new stream for video
			hr = pProfile->CreateNewStream(WMMEDIATYPE_Video,&pStrmCfg);
			BREAK_IF_FAILED(hr);

			hr = pStrmCfg->QueryInterface(IID_IWMMediaProps,reinterpret_cast<void **>(&pMediaProp));
			BREAK_IF_FAILED(hr);

            if(IsCompressedData(0))
            {
                pMediaType = reinterpret_cast<WM_MEDIA_TYPE *>(&m_mtVideo);
            }
            else
            {
                //
			    if(!IsCodecAvailable(pProfileManager,WMMEDIATYPE_Video,WMMEDIASUBTYPE_WMV3,&pType))
			    {
				    hr = E_FAIL;
				    break;
			    }

			    pMediaType  = reinterpret_cast<WM_MEDIA_TYPE *>(pType);
			    // Adjust the video's width, height, framerate, bitrate
			    WMVIDEOINFOHEADER * pvihCodec = reinterpret_cast<WMVIDEOINFOHEADER*>(pMediaType->pbFormat);
			    if(m_mtVideo.formattype == FORMAT_VideoInfo)
			    {

				    VIDEOINFOHEADER *pvihRecord = reinterpret_cast<VIDEOINFOHEADER *>(m_mtVideo.pbFormat);

				    LONG biWidth = pvihRecord->bmiHeader.biWidth;
				    LONG biHeight = pvihRecord->bmiHeader.biHeight;
				    // NOTICE! if we allow more format other than only RGB24, should add this
				    //biWidth = (biWidth>>2)<<2;
				    //biHeight = (biHeight>>2)<<2;

				    pvihCodec->AvgTimePerFrame			= pvihRecord->AvgTimePerFrame;
				    pvihCodec->bmiHeader.biWidth		= biWidth;
				    pvihCodec->bmiHeader.biHeight		= biHeight;
				    pvihCodec->bmiHeader.biBitCount		= pvihRecord->bmiHeader.biBitCount;
				    pvihCodec->bmiHeader.biPlanes		= pvihRecord->bmiHeader.biPlanes;
				    pvihCodec->bmiHeader.biSizeImage	= pvihRecord->bmiHeader.biSizeImage;
				    pvihCodec->dwBitRate				= m_dwVideoBitrate;

				    pvihCodec->rcSource.top = 0;
				    pvihCodec->rcSource.left = 0;
				    pvihCodec->rcSource.right = pvihCodec->bmiHeader.biWidth;
				    pvihCodec->rcSource.bottom = pvihCodec->bmiHeader.biHeight;
				    CopyRect(&pvihCodec->rcTarget,&pvihCodec->rcSource);
			    }
			    else if(m_mtVideo.formattype == FORMAT_VideoInfo2)
			    {
				    VIDEOINFOHEADER2 *pvihRecord = reinterpret_cast<VIDEOINFOHEADER2 *>(m_mtVideo.pbFormat);
				    if(pvihRecord->dwInterlaceFlags & AMINTERLACE_IsInterlaced 
					    || pvihRecord->dwControlFlags & AMCONTROL_USED )
				    {
					    hr = E_INVALIDARG;
					    break;
				    }

				    LONG biWidth = pvihRecord->bmiHeader.biWidth;
				    LONG biHeight = pvihRecord->bmiHeader.biHeight;
				    // NOTICE! if we allow more format other than only RGB24, should add this
				    //biWidth = (biWidth>>2)<<2;
				    //biHeight = (biHeight>>2)<<2;

				    pvihCodec->AvgTimePerFrame			= pvihRecord->AvgTimePerFrame;
				    pvihCodec->bmiHeader.biWidth		= biWidth;
				    pvihCodec->bmiHeader.biHeight		= biHeight;
				    pvihCodec->bmiHeader.biBitCount		= pvihRecord->bmiHeader.biBitCount;
				    pvihCodec->bmiHeader.biPlanes		= pvihRecord->bmiHeader.biPlanes;
				    pvihCodec->bmiHeader.biSizeImage	= pvihRecord->bmiHeader.biSizeImage;
				    pvihCodec->dwBitRate				= m_dwVideoBitrate;

				    pvihCodec->rcSource.top = 0;
				    pvihCodec->rcSource.left = 0;
				    pvihCodec->rcSource.right = pvihCodec->bmiHeader.biWidth;
				    pvihCodec->rcSource.bottom = pvihCodec->bmiHeader.biHeight;
				    CopyRect(&pvihCodec->rcTarget,&pvihCodec->rcSource);
			    }
			    else
			    {
				    hr = E_INVALIDARG;
				    break;
			    }
            }

			hr = pMediaProp->SetMediaType(pMediaType);
			SAFE_RELEASE(pMediaProp);

			hr = pStrmCfg->SetStreamName(TEXT("Video Stream"));
			hr = pStrmCfg->SetConnectionName(TEXT("Video"));
            if (IsCompressedData(0))
            {
                //Whennever VIDEOINFOHEADER2, MPEG2VIDEOINFO and VIDEOINFOHEADER, the following cast is correct
                VIDEOINFOHEADER *pvihRecord = reinterpret_cast<VIDEOINFOHEADER *>(m_mtVideo.pbFormat);
                m_dwVideoBitrate =  pvihRecord->dwBitRate;
            }
			hr = pStrmCfg->SetBitrate(m_dwVideoBitrate);
			hr = pStrmCfg->SetBufferWindow(m_msVideoBufferWindow);
			hr = pStrmCfg->SetStreamNumber(m_wVideoStrmNum);

			hr = pProfile->AddStream(pStrmCfg);
			SAFE_RELEASE(pStrmCfg);
			SAFE_ARRAYDELETE(pType);//allocated in IsCodecAvailable
			BREAK_IF_FAILED(hr);
		}

		// -----------------------------------------------------------------------------------
		// ------------------------- configure audio stream ----------------------------------
		// -----------------------------------------------------------------------------------
		if(m_pAudioPin->IsConnected())
		{
			// create a new stream for audio
			hr = pProfile->CreateNewStream(WMMEDIATYPE_Audio,&pStrmCfg);
			BREAK_IF_FAILED(hr);

			hr = pStrmCfg->QueryInterface(IID_IWMMediaProps,reinterpret_cast<void **>(&pMediaProp));
			BREAK_IF_FAILED(hr);

            if(IsCompressedData(1))
            {
                pMediaType = reinterpret_cast<WM_MEDIA_TYPE *>(&m_mtAudio);
            }
            else
            {

			    // Define the requirement for audio format, such as
			    // samplerate, channels, bitspersample, bitrate
			    // DEMO: 32KHz, 2 Channels, 16 Bits, 48Kbps
			    WAVEFORMATEX desired;
			    WAVEFORMATEX *pRecordWfe = reinterpret_cast<WAVEFORMATEX *>(m_mtAudio.pbFormat);
			    ZeroMemory(&desired, sizeof(WAVEFORMATEX));
			    desired.wFormatTag		= WAVE_FORMAT_PCM;
			    desired.nChannels       = pRecordWfe->nChannels;
			    desired.nSamplesPerSec  = pRecordWfe->nSamplesPerSec;
			    desired.wBitsPerSample  = pRecordWfe->wBitsPerSample;
			    desired.nBlockAlign		= pRecordWfe->nBlockAlign;//desired.wBitsPerSample * desired.nChannels / 8;
			    desired.nAvgBytesPerSec = pRecordWfe->nAvgBytesPerSec;//desired.nBlockAlign * desired.nSamplesPerSec;//mAudioBitrate/ 8;  // 48000 / 8
			    desired.cbSize			= 0;/*it will be ignored becauseof the PCM format*/
			    pType = (BYTE*) &desired;

			    if(!IsCodecAvailable(pProfileManager,WMMEDIATYPE_Audio,WMMEDIASUBTYPE_WMAudioV9,&pType))
			    {
				    if(!IsCodecAvailable(pProfileManager,WMMEDIATYPE_Audio,WMMEDIASUBTYPE_WMAudioV8,&pType))
				    {
					    if(!IsCodecAvailable(pProfileManager,WMMEDIATYPE_Audio,WMMEDIASUBTYPE_WMSP1,&pType))
					    {
						    hr = E_FAIL;
						    break;
					    }
				    }
			    }

			    pMediaType  = (WM_MEDIA_TYPE *) pType;
            }
			hr = pMediaProp->SetMediaType(pMediaType);
			SAFE_RELEASE(pMediaProp);
			BREAK_IF_FAILED(hr);

			hr = pStrmCfg->SetStreamName(TEXT("Audio Stream"));
			hr = pStrmCfg->SetConnectionName(TEXT("Audio"));
            if (IsCompressedData(1))
            {
                WAVEFORMATEX *pRecordWfe = reinterpret_cast<WAVEFORMATEX *>(m_mtAudio.pbFormat);
                m_dwAudioBitrate =  pRecordWfe->nAvgBytesPerSec * 8;
            }
			hr = pStrmCfg->SetBitrate(m_dwAudioBitrate);
			hr = pStrmCfg->SetBufferWindow(m_msAudioBufferWindow);

			hr = pStrmCfg->SetStreamNumber(m_wAudioStrmNum);
			// Add this audio stream to the profile
			hr = pProfile->AddStream(pStrmCfg);

			SAFE_RELEASE(pStrmCfg);
			BREAK_IF_FAILED(hr);
		}

		// -----------------------------------------------------------------------------------
		// ----------------------- configure misc in profile ---------------------------------
		// -----------------------------------------------------------------------------------

		// nothing to do now:)


	} while(false);

	SAFE_RELEASE(pProfileManager);
	SAFE_RELEASE(pStrmCfg);
	SAFE_RELEASE(pMediaProp);

	*ppProfile = pProfile;
	//no need to release pProfile here
	return hr;
}

BOOL CXiaoZaiPushFilter::IsCompressedData(int pinnum)
{
	BOOL bCompressed = FALSE, bVCompressed = TRUE, bACompressed = TRUE;

    //Check video pin
    //if ((m_pVideoPin->IsConnected())&&(m_mtVideo.subtype != WMMEDIASUBTYPE_WMV3)&&(m_mtVideo.subtype != WMMEDIASUBTYPE_MP4S)
    //    &&(m_mtVideo.subtype != WMMEDIASUBTYPE_MPEG2_VIDEO)&&(m_mtVideo.subtype != WMMEDIASUBTYPE_WMV2)&&(m_mtVideo.subtype != WMMEDIASUBTYPE_WMV1))
    if ((m_mtVideo.subtype == MEDIASUBTYPE_RGB24)&&(m_pVideoPin->IsConnected()))
    {
        bVCompressed = FALSE;
    }

    //Check audio pin
    //if ((m_pAudioPin->IsConnected()) && (m_mtAudio.subtype != WMMEDIASUBTYPE_WMAudioV9)&&(m_mtAudio.subtype != WMMEDIASUBTYPE_WMAudioV8)&&(m_mtAudio.subtype != WMMEDIASUBTYPE_MP3))
    if ((m_mtAudio.subtype == MEDIASUBTYPE_PCM)&&(m_pAudioPin->IsConnected()))
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

    default:
        break;
    }
	return bCompressed;

}

HRESULT STDMETHODCALLTYPE CXiaoZaiPushFilter::SetRecordPath(const BSTR outputPath)
{
	m_recordPath = outputPath;
	return S_OK;
}
