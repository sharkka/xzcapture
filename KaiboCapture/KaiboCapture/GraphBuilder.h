#pragma once
#include <DShow.h>
#include <hash_map>
#include "thread.h"
#include <list>

//----------------------------------------------------------------------------------------------------------------------------//
void Msg(TCHAR *szFormat, ...);
#define JIF(x) if (FAILED(hr=(x))) \
	{Msg(TEXT("FAILED(hr=0x%x) in ") TEXT(#x) TEXT("\n\0"), hr); return hr; }

#define LIF(x) if (FAILED(hr=(x))) \
	{Msg(TEXT("FAILED(hr=0x%x) in ") TEXT(#x) TEXT("\n\0"), hr); }
//----------------------------------------------------------------------------------------------------------------------------//
class CGraphBuilder
{
public:
	CGraphBuilder(void);
	~CGraphBuilder(void);

	static std::list<_tstring> CGraphBuilder::GetDeviceList(int dev_type);

	std::list<_tstring> GetDeviceProps();
	
private:
	HRESULT GetCaptureRatio(IBaseFilter* pCapFilter,
		ICaptureGraphBuilder2* pBuild,
		std::list<_tstring>& fmtstr);
	///
	/// The following member variables and functions are designed to support multiple graphbuilders
	///
    static stdext::hash_map<HWND, CGraphBuilder*> m_GraphBuilderMap;
    static OS::CCriticalSection m_MapCs;

    static inline void GetBuilder(HWND hWnd, CGraphBuilder** pBuilder)
    {
        OS::CCriticalSection::AutoLock mylock(m_MapCs);
        stdext::hash_map<HWND, CGraphBuilder*>::iterator it = m_GraphBuilderMap.find(hWnd);
        if (it == m_GraphBuilderMap.end())
            *pBuilder = NULL;
        else
            *pBuilder = it->second;
    }
public:
	void testVolume() {
		HRESULT hr;
		hr = m_pBAud->put_Volume(-3500);
		// Read current volume
		LONG vol = 0;
		hr = m_pBAud->get_Volume(&vol);
	}

private:
	//
	//Methods
	//
	HRESULT _Initialize();
	void _UnInitialize();

	HRESULT RemoveAllFilters(IGraphBuilder *pGraph);
	BOOL IsRendererMultiple();
	BOOL IsRendererFilter(IBaseFilter* pFilter);

	HRESULT SplitFullName(BSTR* videoDevicePath, BSTR* audioDevicePath, BSTR szFullname);
	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

	//
	//Variables
	//
	IGraphBuilder *m_pGraph;
	ICaptureGraphBuilder2 *m_pCapture;

	IBasicAudio* m_pBAud;
	IBasicAudio* m_pBMic;

public:
	ULONG m_ulContentType;
	BSTR m_szVideoSrc;

	//Variables for Snapshot
	LONG m_snapWidth;
	LONG m_snapHeight;
	WORD m_snapBitcount;
	UINT_PTR m_snapTimer;
	BSTR m_szChannelID;
	BSTR m_szFtpUrl;
	BSTR m_szPrefix;
	SHORT m_nShotIndex;

};
