#include "stdafx.h"
#include "GraphBuilder.h"
#include <gdiplus.h>
#include <WinInet.h>
#include <string>
#include <Dvdmedia.h>
#include "Streams.h"
#include "MyDef.h"
#include "TString.h"

#include "ComHelper.h"
#include "smartptr.h"
//#include "dmoenum/namedguid.h"

#define WM_ICON_NOTIFY		WM_APP + 1
#define IDTIMER_EVENT		WM_APP + 2
#define IDTIMER_CONTROL		WM_APP + 3
#define WM_GRAPHNOTIFY		WM_APP + 4
#define IDTIMER_SNAPSHOT	WM_APP + 5
#define SNAPSHOTFREQUENCY 10

using namespace Gdiplus;
using namespace std;

#define FCC_FORMAT_DIVX_MPEG4 mmioFOURCC('D','X','5','0')
#define FCC_FORMAT_WIS_MPEG4 mmioFOURCC('W','M','P',0x04)
#define FCC_FORMAT_MICROSOFT_MPEG4 mmioFOURCC('M','P','4','S')
#define FCC_FORMAT_SIGMA_MPEG4 mmioFOURCC('R','M','P','4')
#define FCC_FORMAT_H263 mmioFOURCC('H','2','6','3')
#define FCC_FORMAT_MOTION_JPEG mmioFOURCC('M','J','P','G')
#define FCC_FORMAT_RCC_MPEG4 mmioFOURCC('D','I','V','X')



void Msg(TCHAR *szFormat, ...)
{
	TCHAR szBuffer[1024];  // Large buffer for long filenames or URLs
	const size_t NUMCHARS = sizeof(szBuffer) / sizeof(szBuffer[0]);
	const int LASTCHAR = NUMCHARS - 1;

	// Format the input string
	va_list pArgs;
	va_start(pArgs, szFormat);

	// Use a bounded buffer size to prevent buffer overruns.  Limit count to
	// character size minus one to allow for a NULL terminating character.
	(void)StringCchVPrintf(szBuffer, NUMCHARS - 1, szFormat, pArgs);
	va_end(pArgs);

	// Ensure that the formatted string is NULL-terminated
	szBuffer[LASTCHAR] = TEXT('\0');

	// Display a message box with the formatted string
	MessageBox(NULL, szBuffer, TEXT("Windowless Sample"), MB_OK);
}

inline BOOL IsGrfFile(BSTR szFilename)
{
	PTSTR ext = PathFindExtension(szFilename);
	_tcslwr_s(ext, _tclen(ext));
	return (_tcscmp(ext, L".grf") == 0 ? TRUE : FALSE);
}
inline ULONG JudgeContType(BSTR szFullname)
{
	if (_tcscmp(PathFindExtension(szFullname), L"") != 0)
	{
		if (IsGrfFile(szFullname))
		{
			return CT_GRF_CONTENT;
		}
		else
		{
			return CT_FILE_CONTENT;
		}
	}
	else
	{
		return CT_LIVE_CONTENT;
	}
}


///
/// Static Variables 
///
stdext::hash_map<HWND, CGraphBuilder*> CGraphBuilder::m_GraphBuilderMap;
OS::CCriticalSection CGraphBuilder::m_MapCs;

CGraphBuilder::CGraphBuilder(void)
{
	m_pGraph = NULL;
	m_pCapture = NULL;

	m_ulContentType = CT_UNDEFINE_CONTENT;
	m_szVideoSrc = SysAllocString(L"");

	m_snapTimer = 0;

	m_szChannelID = SysAllocString(L"");

	m_szFtpUrl = SysAllocString(L"");
	m_szPrefix = SysAllocString(L"");

	_Initialize();
}

CGraphBuilder::~CGraphBuilder(void)
{
	_UnInitialize();

	SysFreeString(m_szChannelID);

	SysFreeString(m_szFtpUrl);
	SysFreeString(m_szPrefix);

	SysFreeString(m_szVideoSrc);
}

HRESULT CGraphBuilder::_Initialize()
{
	//////////////////////////////////////////////////////////////////////////
	// Above is for SVS

	// Create the filter graph
	HRESULT hr = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
		IID_IGraphBuilder, (void **) &m_pGraph);
	RETURN_IF_FAILED(hr);

	// Obtain interfaces for media control and Video Window
	/*hr = m_pGraph->QueryInterface(IID_IMediaControl,(LPVOID *) &m_pMC);
	RETURN_IF_FAILED(hr);

	hr = m_pGraph->QueryInterface(IID_IMediaEvent, (LPVOID *) &m_pME);
	RETURN_IF_FAILED(hr);

	hr = m_pGraph->QueryInterface(IID_IMediaSeeking, (void **)&m_pMS);
	RETURN_IF_FAILED(hr);*/

	hr = CoCreateInstance (CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC_SERVER,
		IID_ICaptureGraphBuilder2, (void **) &m_pCapture);
	RETURN_IF_FAILED(hr);


	//////////////////////////////////////////////////////////////////////////
	// Set video window. 

	// Attach the filter graph to the capture graph
	hr = m_pCapture->SetFiltergraph(m_pGraph);
	if (FAILED(hr))
	{
		ATLTRACE(L"Failed to set capture filter graph!");
		RETURN_IF_FAILED(hr);
	}

	/*hr = m_pME->SetNotifyWindow((OAHWND)m_hWnd, WM_GRAPHNOTIFY, 0);
	if (FAILED(hr))
	{
		ATLTRACE(L"Couldn't Register Window Message!");
		return hr;
	}*/

	JIF(m_pGraph->QueryInterface(IID_IBasicAudio, (void **)&m_pBAud));
	//JIF(m_pCapture->QueryInterface(IID_IBasicAudio, (void **)&pBMic));

	/*LONG vol = 0;
	hr = m_pBAud->put_Volume(-3500);
	hr = m_pBAud->get_Volume(&vol);
	printf("audio volume: %ld\n, vol");
	//pBMic->get_Volume(&vol);
	//printf("micphone volume %ld\n", vol);*/

	return hr;
}

HRESULT CGraphBuilder::RemoveAllFilters(IGraphBuilder *pGraph)
{
	//Begin to remove. 
	IEnumFilters *pEnum = NULL;

	HRESULT hr = pGraph->EnumFilters(&pEnum);
	RETURN_IF_FAILED(hr);

	IBaseFilter *filters[1024] = {0};
	ULONG fetched=0;

	do 
	{
		hr = pEnum->Next(1024, filters, &fetched);
		BREAK_IF_FAILED(hr);

		for(ULONG i = 0 ; i < fetched ; i++)
		{
#ifdef _DEBUG
			FILTER_INFO fi;
			hr = filters[i]->QueryFilterInfo(&fi);
			CONTINUE_IF_FAILED(hr);
#endif

			hr = pGraph->RemoveFilter(filters[i]);
			CONTINUE_IF_FAILED(hr);
		}

	} while(fetched >= 1024);

	for (ULONG i = 0; i < fetched; i++)
		SAFE_RELEASE(filters[i]);
	SAFE_RELEASE(pEnum);

	return hr;
}

void CGraphBuilder::_UnInitialize()
{
	RemoveAllFilters(m_pGraph);

	// Release DirectShow interfaces
	SAFE_RELEASE(m_pCapture);
	SAFE_RELEASE(m_pGraph);
}

list<_tstring> CGraphBuilder::GetDeviceList(int dev_type)
{
	// Create the capture graph builder
	HRESULT hr = S_OK;
	IPropertyBag* pPropBag = NULL;
	IMoniker* pMoniker = NULL;
	list<_tstring> mylist;

	// Create the system device enumerator
	CComPtr <ICreateDevEnum> pDevEnum = NULL;
	// Create an enumerator for the capture devices
	CComPtr <IEnumMoniker> pClassEnum = NULL;

	do
	{
		hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
			IID_ICreateDevEnum, (void **)&pDevEnum);
		if (FAILED(hr))
		{
			ATLTRACE(L"Couldn't create system enumerator!");
			BREAK_IF_FAILED(hr);
		}

		//////////////////////////////////////////////////////////////////////////
		// Enum Video Device. 
		if (dev_type == 0 )
			hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
		else
			hr = hr = pDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pClassEnum, 0);
		if (FAILED(hr))
		{
			ATLTRACE(L"Couldn't create class enumerator!");
			BREAK_IF_FAILED(hr);
		}

		// If there are no enumerators for the requested type, then 
		// CreateClassEnumerator will succeed, but pClassEnum will be NULL.
		if (pClassEnum == NULL)
		{
			ATLTRACE(L"Couldn't create class enumerator!");
			hr = E_FAIL;
			break;
		}

		// Use the first video capture device on the device list.
		// Note that if the Next() call succeeds but there are no monikers,
		// it will return S_FALSE (which is not a failure).  Therefore, we
		// check that the return code is S_OK instead of using SUCCEEDED() macro.
		while (pClassEnum->Next(1, &pMoniker, NULL) == S_OK)
		{

			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag,
				(void**)(&pPropBag));						

			//////////////////////////////////////////////////////////////////////////
			//Failed
			//////////////////////////////////////////////////////////////////////////
			if (FAILED(hr))
			{
				ATLTRACE(L"Couldn't bind moniker to filter object!");
				CONTINUE_IF_FAILED(hr);
			}

			//////////////////////////////////////////////////////////////////////////
			//Succeed
			//////////////////////////////////////////////////////////////////////////

			//For Capture Device name
			VARIANT varName;
			VariantInit(&varName);
			hr = pPropBag->Read(L"FriendlyName", &varName, 0);
			if (SUCCEEDED(hr))
			{
				// Add it to the application's list box.
				USES_CONVERSION;
				_tstring mystr = OS::StringW2T(varName.bstrVal);
				mylist.push_back(mystr);
			}
			VariantClear(&varName);

			SAFE_RELEASE(pPropBag);
			SAFE_RELEASE(pMoniker);
		}

		SAFE_RELEASE(pPropBag);
		SAFE_RELEASE(pMoniker);
	} while (false);

	//do release resources here
	SAFE_RELEASE(pPropBag);
	SAFE_RELEASE(pMoniker);
	return mylist;
}
/******************************************************************************************************************************
* Function    : GetDeviceProps
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/29 12:40:13
* Modify Date : 2016/03/29 12:40:13
******************************************************************************************************************************/
list<_tstring> CGraphBuilder::GetDeviceProps()
{
	HRESULT hr = -1;
	list<_tstring> fmtlist;
	IMoniker* pMoniker = NULL;
	// Create the system device enumerator
	CComPtr <ICreateDevEnum> pDevEnum = NULL;
	// Create an enumerator for the capture devices
	CComPtr <IEnumMoniker> pClassEnum = NULL;

	IBaseFilter *pCap;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
		IID_ICreateDevEnum, (void **)&pDevEnum);
	if (FAILED(hr))
	{
		ATLTRACE(L"Couldn't create system enumerator!");
		return fmtlist;
	}

	//////////////////////////////////////////////////////////////////////////
	// Enum Video Device. 
	hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);

	if (FAILED(hr))
	{
		ATLTRACE(L"Couldn't create class enumerator!");
		return fmtlist;
	}

	// If there are no enumerators for the requested type, then 
	// CreateClassEnumerator will succeed, but pClassEnum will be NULL.
	if (pClassEnum == NULL)
	{
		ATLTRACE(L"Couldn't create class enumerator!");
		hr = E_FAIL;
		return fmtlist;
	}
	while (pClassEnum->Next(1, &pMoniker, NULL) == S_OK)
	{
		pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pCap);

		GetCaptureRatio(pCap, m_pCapture, fmtlist);
	}


	
	return fmtlist;
}
/******************************************************************************************************************************
* Function    : GetCaptureRatio
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/29 12:40:02
* Modify Date : 2016/03/29 12:40:02
******************************************************************************************************************************/
HRESULT CGraphBuilder::GetCaptureRatio(IBaseFilter* pCapFilter, 
										ICaptureGraphBuilder2* pBuild,
										list<_tstring>& fmtstr) {
	HRESULT hr;

	SmartPtr<IAMStreamConfig> pam;
	hr = pBuild->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
		pCapFilter, IID_IAMStreamConfig, reinterpret_cast<void**>(&pam)); // 得到媒体控制接口

	int nCount = 0;
	int nSize = 0;
	hr = pam->GetNumberOfCapabilities(&nCount, &nSize);

	char SCC[2048] = { 0 };
	// 判断是否为视频信息
	char fmt[128] = { 0 };
	if (sizeof(VIDEO_STREAM_CONFIG_CAPS) == nSize) {
		for (int i = 0; i < nCount; i++) {
			//VIDEO_STREAM_CONFIG_CAPS scc;
			AM_MEDIA_TYPE* pmmt;

			hr = pam->GetStreamCaps(i, &pmmt, reinterpret_cast<BYTE*>(&SCC));

			if (pmmt->formattype == FORMAT_VideoInfo) {
				if (pmmt->subtype == MEDIASUBTYPE_YV12 || pmmt->subtype == MEDIASUBTYPE_MJPG) {
					VIDEOINFOHEADER* pvih = reinterpret_cast<VIDEOINFOHEADER*>(pmmt->pbFormat);
					int nwidth = pvih->bmiHeader.biWidth; // 得到采集的宽 
					int nheight = pvih->bmiHeader.biHeight; //　得到采集的高
					sprintf(fmt, ("%dx%d fps=%2.2f"), nwidth, nheight, 1000000.0 / (pvih->AvgTimePerFrame * 0.1));
					fmtstr.push_back(OS::StringA2T(fmt));
					//printf("%dx%d fps=%2.2f\n", nwidth, nheight, 1000000.0 / (pvih->AvgTimePerFrame * 0.1));
					//printf("biCompression: %ld\n", pvih->bmiHeader.biCompression);
				}
			}
		}
	}

	return (hr);
}
HRESULT GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
    IEnumPins  *pEnum = NULL;
    IPin       *pPin = NULL;
    HRESULT    hr;

    if (ppPin == NULL)
    {
        return E_POINTER;
    }

    hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return hr;
    }
    while(pEnum->Next(1, &pPin, 0) == S_OK)
    {
        PIN_DIRECTION PinDirThis;
        hr = pPin->QueryDirection(&PinDirThis);
        if (FAILED(hr))
        {
            pPin->Release();
            pEnum->Release();
            return hr;
        }
        if (PinDir == PinDirThis)
        {
            // Found a match. Return the IPin pointer to the caller.
            *ppPin = pPin;
            pEnum->Release();
            return S_OK;
        }
        // Release the pin for the next time through the loop.
        pPin->Release();
    }
    // No more pins. We did not find a match.
    pEnum->Release();
    return E_FAIL;  
}



int CGraphBuilder::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = reinterpret_cast<ImageCodecInfo*>(new BYTE[size]);
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			SAFE_ARRAYDELETE(pImageCodecInfo);
			return j;  // Success
		}    
	}

	SAFE_ARRAYDELETE(pImageCodecInfo);
	return -1;  // Failure
}


HRESULT CGraphBuilder::SplitFullName(BSTR *videoDevicePath, 
									 BSTR *audioDevicePath, 
									 BSTR szFullname)
{
	wstring str(szFullname);
	wstring::size_type len = str.length();
	wstring::size_type index = str.find('|', 0);

	SysReAllocString(videoDevicePath, str.substr(0, index).c_str());
	SysReAllocString(audioDevicePath, 
				str.substr(index + 1, len - index - 2).c_str());

	return S_OK;
}

BOOL CGraphBuilder::IsRendererMultiple()
{
	IEnumFilters *pEnum = NULL;
	int cRenderer = 0;

	HRESULT hr = m_pGraph->EnumFilters(&pEnum);
	if(FAILED(hr))
	{
		return TRUE;		// This means somethings error.	
							// This won't happen in fact. 
	}

	IBaseFilter *filters[1024] = {0};

	ULONG fetched = 0;
	do 
	{
		hr = pEnum->Next(1024,filters,&fetched);
		BREAK_IF_FAILED(hr);

		for(ULONG i = 0 ; i < fetched ; i++)
		{

#ifdef _DEBUG
			FILTER_INFO fi;
			hr = filters[i]->QueryFilterInfo(&fi);
			if (FAILED(hr))
			{
				SAFE_RELEASE(filters[i]);
				continue;
			}
#endif
			if(IsRendererFilter(filters[i]))
			{
				cRenderer++;
			}

			SAFE_RELEASE(filters[i]);
		}

	} while(fetched >= 1024);

	SAFE_RELEASE(pEnum);

	return (cRenderer > 1);
}

BOOL CGraphBuilder::IsRendererFilter(IBaseFilter *pFilter)
{
	HRESULT hr = S_OK;
	IEnumPins *pPins = NULL;
	IPin *pins[8] = {0};
	ULONG cPins = 0;
	BOOL ret = TRUE;

	do
	{
		hr = pFilter->EnumPins(&pPins);
		BREAK_IF_FAILED(hr);

		hr = pPins->Next(8, pins, &cPins);
		BREAK_IF_FAILED(hr);

		for(ULONG i = 0 ; i < cPins ; i++)
		{
			PIN_INFO info;

			hr = pins[i]->QueryPinInfo(&info);

			if(info.dir == PINDIR_OUTPUT)
			{
				info.pFilter->Release();
				ret = FALSE;
				break;
			}
			info.pFilter->Release();
		}

	}while(FALSE);

	for(ULONG i = 0 ; i < cPins ; i++)
		SAFE_RELEASE(pins[i]);
	SAFE_RELEASE(pPins);

	return ret;
}
