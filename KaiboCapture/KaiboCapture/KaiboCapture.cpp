
// KaiboCapture.cpp : 定義應用程式的類別行為。
//

#include "stdafx.h"
#include "KaiboCapture.h"
#include "KaiboCaptureDlg.h"
#include "GraphBuilder.h"
#include "LoginDialog.h"
#include "curl/curl.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CKaiboCaptureApp

BEGIN_MESSAGE_MAP(CKaiboCaptureApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CKaiboCaptureApp 建構

CKaiboCaptureApp::CKaiboCaptureApp()
{
	// 支援重新啟動管理員
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO:  在此加入建構程式碼，
	// 將所有重要的初始設定加入 InitInstance 中
}


// 僅有的一個 CKaiboCaptureApp 物件

CKaiboCaptureApp theApp;


// CKaiboCaptureApp 初始設定

BOOL CKaiboCaptureApp::InitInstance()
{
	// 假如應用程式資訊清單指定使用 ComCtl32.dll 6 (含) 以後版本，
	// 來啟動視覺化樣式，在 Windows XP 上，則需要 InitCommonControls()。
	// 否則任何視窗的建立都將失敗。
	//InitCommonControls();

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 設定要包含所有您想要用於應用程式中的
	// 通用控制項類別。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// 建立殼層管理員，以防對話方塊包含
	// 任何殼層樹狀檢視或殼層清單檢視控制項。
	CShellManager *pShellManager = new CShellManager;
	// 啟動 [Windows 原生] 視覺化管理員可啟用 MFC 控制項中的主題
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	// 標準初始設定
	// 如果您不使用這些功能並且想減少
	// 最後完成的可執行檔大小，您可以
	// 從下列程式碼移除不需要的初始化常式，
	// 變更儲存設定值的登錄機碼
	// TODO:  您應該適度修改此字串
	// (例如，公司名稱或組織名稱)
	SetRegistryKey(_T("本機 AppWizard 所產生的應用程式"));

#ifdef _DEBUG
	CMemoryState mystate;
	mystate.Checkpoint();
#endif
	//============================================================================================================================//
	curl_global_init(CURL_GLOBAL_ALL);
	CLoginDialog loginDialog;
#ifdef LOGIN_MODE
	INT_PTR nLogin = loginDialog.DoModal();

	if (nLogin == IDOK) {
		// 连接应用服务器，以完成登陆认证
		if (0 == loginDialog.getLoginStatus()) {
			return FALSE;
		}
	} else if (nLogin == IDCANCEL) {
		return FALSE;
	}
#else
	//m_rtmpUrl = "rtmp://192.168.177.112:1935/live/stream1";
	//m_rtmpUrl = "rtmp://localhost:1935/live/stream1";
	m_rtmpUrl = "rtmp://livexz.xiaozaiwh.com:1935/live/streama";
#endif
	//============================================================================================================================//

	//Initialize the FFMPEG
	CXiaozaiFilters::Init(XZ_LOG_VERBOSE);

	CKaiboCaptureDlg dlg;
#ifdef LOGIN_MODE
	dlg.setUsername(loginDialog.getUsername());
	dlg.setPassword(loginDialog.getPassword());
	dlg.setServAddress(m_servAddress);
	dlg.setPort(m_servPort);
	dlg.setVideocastId(loginDialog.getVideocastId());
#endif
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	
	if (nResponse == IDOK)
	{
		// TODO:  在此放置於使用 [確定] 來停止使用對話方塊時
		// 處理的程式碼
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO:  在此放置於使用 [取消] 來停止使用對話方塊時
		// 處理的程式碼
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "警告: 對話方塊建立失敗，因此，應用程式意外終止。\n");
		TRACE(traceAppMsg, 0, "警告: 如果您要在對話方塊上使用 MFC 控制項，則無法 #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS。\n");
	}

	// 刪除上面所建立的殼層管理員。
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	//Uninitialize the FFMPEG
	CXiaozaiFilters::Uninit();
	curl_global_cleanup();

	// 因為已經關閉對話方塊，傳回 FALSE，所以我們會結束應用程式，
	// 而非提示開始應用程式的訊息。
	CoUninitialize();
#ifdef _DEBUG
	mystate.DumpAllObjectsSince();
#endif
	return FALSE;

}


void CKaiboCaptureApp::Test1()
{

	CGraphBuilder *mygraph = new CGraphBuilder();
	mygraph->GetDeviceList(0);
	mygraph->GetDeviceList(1);
	delete mygraph;
}
