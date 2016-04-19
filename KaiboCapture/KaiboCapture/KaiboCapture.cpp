
// KaiboCapture.cpp : ���x���ó�ʽ��e�О顣
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


// CKaiboCaptureApp ����

CKaiboCaptureApp::CKaiboCaptureApp()
{
	// ֧Ԯ�����ӹ���T
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO:  �ڴ˼��뽨����ʽ�a��
	// ��������Ҫ�ĳ�ʼ�O������ InitInstance ��
}


// �H�е�һ�� CKaiboCaptureApp ���

CKaiboCaptureApp theApp;


// CKaiboCaptureApp ��ʼ�O��

BOOL CKaiboCaptureApp::InitInstance()
{
	// ���瑪�ó�ʽ�YӍ���ָ��ʹ�� ComCtl32.dll 6 (��) ����汾��
	// �톢��ҕ�X����ʽ���� Windows XP �ϣ��t��Ҫ InitCommonControls()��
	// ��t�κ�ҕ���Ľ�������ʧ����
	//InitCommonControls();

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// �O��Ҫ������������Ҫ��춑��ó�ʽ�е�
	// ͨ�ÿ����e��
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// �������ӹ���T���Է���Ԓ���K����
	// �κΚ��Ә��zҕ�򚤌���Ιzҕ����헡�
	CShellManager *pShellManager = new CShellManager;
	// ���� [Windows ԭ��] ҕ�X������T�Ɇ��� MFC ������е����}
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	// �˜ʳ�ʼ�O��
	// �������ʹ���@Щ���܁K����p��
	// ������ɵĿɈ��Йn��С��������
	// �����г�ʽ�a�Ƴ�����Ҫ�ĳ�ʼ����ʽ��
	// ׃�������O��ֵ�ĵ�䛙C�a
	// TODO:  ����ԓ�m���޸Ĵ��ִ�
	// (���磬��˾���Q��M�����Q)
	SetRegistryKey(_T("���C AppWizard ���a���đ��ó�ʽ"));

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
		// ����Ӧ�÷�����������ɵ�½��֤
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
		// TODO:  �ڴ˷����ʹ�� [�_��] ��ֹͣʹ�Ì�Ԓ���K�r
		// ̎��ĳ�ʽ�a
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO:  �ڴ˷����ʹ�� [ȡ��] ��ֹͣʹ�Ì�Ԓ���K�r
		// ̎��ĳ�ʽ�a
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "����: ��Ԓ���K����ʧ������ˣ����ó�ʽ����Kֹ��\n");
		TRACE(traceAppMsg, 0, "����: �����Ҫ�ڌ�Ԓ���K��ʹ�� MFC ����헣��t�o�� #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS��\n");
	}

	// �h�������������Ě��ӹ���T��
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	//Uninitialize the FFMPEG
	CXiaozaiFilters::Uninit();
	curl_global_cleanup();

	// ����ѽ��P�]��Ԓ���K������ FALSE�������҂����Y�����ó�ʽ��
	// ������ʾ�_ʼ���ó�ʽ��ӍϢ��
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
