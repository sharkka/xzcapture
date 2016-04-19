//== CODE ADD BY ANYZ =================================================================//
// Date: 2016/04/19 13:19:19
// Despcription: test for visual studio git 

//== CODE END =========================================================================//
// KaiboCaptureDlg.cpp : 實作檔
//

#include "stdafx.h"
#include "KaiboCapture.h"
#include "KaiboCaptureDlg.h"
#include "afxdialogex.h"
#include "GraphBuilder.h"
#include "resource.h"
#include <list>
#include <json/json.h>

#include "PreviewDlg.h"
#include "XzConfig.h"
#include "AudioVolume.h"
#include "openssl/md5.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//----------------------------------------------------------------------------------------------------------------------------//
// Finally, will be move to a header file
#define POSTURL_START_VIDEO				"http://%s:%d/xiaozaikaibo/app/secondaryTwo/cuser/updateStartTimeVideo"
#define POSTURL_END_VIDEO				"http://%s:%d/xiaozaikaibo/app/secondaryTwo/cuser/updateEndTimeVideo"
#define POSTURL_VIDEO_PERMISSION		"http://%s:%d/xiaozaikaibo/app/secondaryTwo/cuser/getByVideoId"
//----------------------------------------------------------------------------------------------------------------------------//
#define POSTFIELDS_START_ENCRYPTVIDEO	"voideId=%s&videoName=%s&startTime=%lld&isInteractive=%d&isLock=1&passwd=%s"
#define POSTFIELDS_START_PLAINVIDEO		"voideId=%s&videoName=%s&startTime=%lld&isInteractive=%d&isLock=0"
#define POSTFIELDS_END_VIDEO			"voideId=%s&endTime=%lld"
#define POSTFIELDS_VIDEO_PERMISSION		"voideId=%s"
//----------------------------------------------------------------------------------------------------------------------------//
#define APPSERVER_RESPONSE_CODE_SUCCESS			1000	// 成功

extern std::string utf82UnicodeString(const char* utf, size_t *unicode_number);


// CKaiboCaptureDlg 對話方塊

#define WM_VIDEO_UPDATE		WM_USER+100
#define WM_LOCAL_PREVIEW	WM_USER+200
#define WM_RTMP_PREVIEW		WM_USER+300

CKaiboCaptureDlg::CKaiboCaptureDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CKaiboCaptureDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CKaiboCaptureDlg::~CKaiboCaptureDlg()
{
	if (m_AVController) {
		delete m_AVController;
		m_AVController = NULL;
	}
	if (m_previewLocalController) {
		delete m_previewLocalController;
		m_previewLocalController = NULL;
	}
	if (m_previewRtmpController) {
		delete m_previewRtmpController;
		m_previewRtmpController = NULL;
	}
}

void CKaiboCaptureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CKaiboCaptureDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//ON_MESSAGE
	ON_BN_CLICKED(IDC_START_VIDEO, &CKaiboCaptureDlg::OnBnClickedStartVideo)
	ON_MESSAGE(WM_VIDEO_UPDATE, &CKaiboCaptureDlg::OnVideoUpdate)
	ON_MESSAGE(WM_LOCAL_PREVIEW, &CKaiboCaptureDlg::OnVideoUpdate)
	ON_MESSAGE(WM_RTMP_PREVIEW, &CKaiboCaptureDlg::OnVideoUpdate)


	ON_BN_CLICKED(IDC_STOP_VIDEO, &CKaiboCaptureDlg::OnBnClickedStopVideo)
	ON_STN_CLICKED(IDC_TESTVIDEO, &CKaiboCaptureDlg::OnStnClickedTestvideo)
	ON_BN_CLICKED(IDOK, &CKaiboCaptureDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_VIDEO_SAVE_PATH, &CKaiboCaptureDlg::OnBnClickedVideoSavePath)
	ON_BN_CLICKED(IDC_PAUSE_VIDEO, &CKaiboCaptureDlg::OnBnClickedPauseVideo)
	ON_BN_CLICKED(IDC_PREVIEW, &CKaiboCaptureDlg::OnBnClickedPreview)
	ON_BN_CLICKED(IDC_MEDIA_SWITCH, &CKaiboCaptureDlg::OnBnClickedMediaSwitch)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_RECORD_VIDEO, &CKaiboCaptureDlg::OnBnClickedRecordVideo)
	ON_BN_CLICKED(IDC_CONFIG, &CKaiboCaptureDlg::OnBnClickedConfig)
	ON_BN_CLICKED(IDC_VIDEO_ENCRYPT, &CKaiboCaptureDlg::OnBnClickedVideoEncrypt)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

static char *hexstr(unsigned char *buf, int len)
{
	const char *set = "0123456789abcdef";
	static char str[65], *tmp;
	unsigned char *end;
	if (len > 32)
		len = 32;
	end = buf + len;
	tmp = &str[0];
	while (buf < end)
	{
		*tmp++ = set[(*buf) >> 4];
		*tmp++ = set[(*buf) & 0xF];
		buf++;
	}
	*tmp = '\0';
	return str;
}
// CKaiboCaptureDlg 訊息處理常式

BOOL CKaiboCaptureDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));

	_CrtDumpMemoryLeaks();


	// 設定此對話方塊的圖示。當應用程式的主視窗不是對話方塊時，
	// 框架會自動從事此作業
	SetIcon(m_hIcon, TRUE);			// 設定大圖示
	SetIcon(m_hIcon, FALSE);		// 設定小圖示

	// TODO:  在此加入額外的初始設定

	GetDlgItem(IDC_START_VIDEO)->EnableWindow(TRUE);
	GetDlgItem(IDC_STOP_VIDEO)->EnableWindow(FALSE);

	m_play_status = 0;
	m_preview_status = 0;
	m_record_status = 0;
	GetDlgItem(IDC_PAUSE_VIDEO)->EnableWindow(FALSE);

	m_allow_comment = 1;
	m_video_encrypt = 0;
	((CButton*)GetDlgItem(IDC_ALLOW_COMMENT))->SetCheck(m_allow_comment);
	((CButton*)GetDlgItem(IDC_VIDEO_ENCRYPT))->SetCheck(m_video_encrypt);
	GetDlgItem(IDC_CRYPT_KEY)->EnableWindow(FALSE);

	//m_video_key = "123456";

	SetDlgItemText(IDC_RECORD_VIDEO, _T("录像(&R)"));
	SetDlgItemText(IDC_START_VIDEO, _T("直播(&B)"));

	setDeviceList();
	setDevicesFormatAvaliable();
	setRtmpUrl();

	CSliderCtrl*	pSCVolume = (CSliderCtrl*)GetDlgItem(IDC_SLD_VOLUME);
	CSliderCtrl*	pSCMicphone = (CSliderCtrl*)GetDlgItem(IDC_SLD_MICPHONE);
	pSCVolume->SetRange(0, 100);
	pSCMicphone->SetRange(0, 100);

	CAudioVolume cav;
	float val = cav.GetSystemVolume();
	if (val < 100 && val > 0)
		pSCVolume->SetPos(100 * val);
	
	val = cav.GetSystemMicVolume();
	if (val < 100 && val > 0)
		pSCMicphone->SetPos(100 * val);

	m_connectRtmpStatus = 0;
	// for debug
	SetDlgItemText(IDC_EDIT_VIDEO_SAVE_PATH, _T("d:\\temp"));
	SetDlgItemText(IDC_VIDEOCAST_NAME, _T("请在这里指定直播视频主题"));
	SetDlgItemText(IDC_CRYPT_KEY, _T("123456"));
	//GetDlgItem(IDC_MEDIA_SWITCH)->EnableWindow(FALSE);

	char pas[32] = "123456";
	unsigned char md[16] = { 0 };
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, pas, strlen(pas));
	MD5_Final(md, &ctx);

	printf("%s\n", hexstr(md, MD5_DIGEST_LENGTH));

	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}
/******************************************************************************************************************************
* Function    : setDeviceList
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/29 12:40:53
* Modify Date : 2016/03/29 12:40:53
******************************************************************************************************************************/
void CKaiboCaptureDlg::setDeviceList()
{
	m_audiodecs = CGraphBuilder::GetDeviceList(1);
	m_videodecs = CGraphBuilder::GetDeviceList(0);

	CComboBox* pVideoDeviceList = ((CComboBox*)GetDlgItem(IDC_VIDEO_DEVICE_LIST));
	if (pVideoDeviceList) {
		if (m_videodecs.size() > 0)
		for (std::list<_tstring>::iterator pos = m_videodecs.begin();
			pos != m_videodecs.end(); ++pos) {
			pVideoDeviceList->AddString(pos->c_str());
		}
		pVideoDeviceList->SetCurSel(0);
	}
	CComboBox* pAudioDeviceList = ((CComboBox*)GetDlgItem(IDC_AUDIO_DEVICE_LIST));
	if (pAudioDeviceList) {
		if (m_audiodecs.size() > 0)
		for (std::list<_tstring>::iterator pos = m_audiodecs.begin();
			pos != m_audiodecs.end(); ++pos) {
			pAudioDeviceList->AddString(pos->c_str());
		}
		pAudioDeviceList->SetCurSel(0);
	}
}
/******************************************************************************************************************************
* Function    : setVideoDeviceList
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/29 12:41:06
* Modify Date : 2016/03/29 12:41:06
******************************************************************************************************************************/
void CKaiboCaptureDlg::setVideoDeviceList(CWnd* pwnd)
{
	std::list<_tstring> videodecs = CGraphBuilder::GetDeviceList(0);
	CComboBox* pVideoDeviceList = (CComboBox*)pwnd;
	if (pVideoDeviceList) {
		if (videodecs.size() > 0)
		for (std::list<_tstring>::iterator pos = videodecs.begin();
			pos != videodecs.end(); ++pos) {
			pVideoDeviceList->AddString(pos->c_str());
		}
		pVideoDeviceList->SetCurSel(0);
	}
}
/******************************************************************************************************************************
* Function    : setAudioDeviceList
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/29 12:41:24
* Modify Date : 2016/03/29 12:41:24
******************************************************************************************************************************/
void CKaiboCaptureDlg::setAudioDeviceList(CWnd* pwnd)
{
	std::list<_tstring> audiodecs = CGraphBuilder::GetDeviceList(1);
	CComboBox* pAudioDeviceList = (CComboBox*)pwnd;
	if (pAudioDeviceList) {
		if (audiodecs.size() > 0)
		for (std::list<_tstring>::iterator pos = audiodecs.begin();
			pos != audiodecs.end(); ++pos) {
			pAudioDeviceList->AddString(pos->c_str());
		}
		pAudioDeviceList->SetCurSel(0);
	}
}
/******************************************************************************************************************************
* Function    : setDevicesFormatAvaliable
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/29 12:21:24
* Modify Date : 2016/03/29 12:21:24
******************************************************************************************************************************/
void CKaiboCaptureDlg::setDevicesFormatAvaliable()
{
	// 设置每种视频设备的显示能力
	CGraphBuilder gb;
	std::list<_tstring> fmtlist = gb.GetDeviceProps();

	CComboBox* pDeviceFormatList = ((CComboBox*)GetDlgItem(IDC_FORMAT_LIST));
	if (pDeviceFormatList) {
		if (fmtlist.size() > 0)
		for (std::list<_tstring>::iterator pos = fmtlist.begin();
			pos != fmtlist.end(); ++pos) {
			pDeviceFormatList->AddString(pos->c_str());
		}
		pDeviceFormatList->SetCurSel(0);
	}
}

void CKaiboCaptureDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 繪製的裝置內容

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 將圖示置中於用戶端矩形
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 描繪圖示
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 當使用者拖曳最小化視窗時，
// 系統呼叫這個功能取得游標顯示。
HCURSOR CKaiboCaptureDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CKaiboCaptureDlg::setRtmpUrl()
{
	CKaiboCaptureApp* theApp = (CKaiboCaptureApp*)AfxGetApp();
	m_rtmpUrl = theApp->m_rtmpUrl;
}

void CKaiboCaptureDlg::makedir(CString path)
{
	if (!PathFileExists(path)) {
		CreateDirectory(path, NULL);
	}
}
void CKaiboCaptureDlg::OnBnClickedStartVideo()
{
	CString savePath;
	GetDlgItemText(IDC_EDIT_VIDEO_SAVE_PATH, savePath);
	if (savePath.IsEmpty()) {
		AfxMessageBox(_T("请选择录像存储路径"));
		return;
	}

	makedir(savePath);
	
	CString videocastName;
	GetDlgItemText(IDC_VIDEOCAST_NAME, videocastName);
	if (videocastName.IsEmpty()) {
		AfxMessageBox(_T("请指定直播视频题目"));
		return;
	}

	if (m_AVController == NULL)
		m_AVController = new CXiaozaiFilters();

	std::list<_tstring> audiodecs= CGraphBuilder::GetDeviceList(1);
	std::list<_tstring> videodecs = CGraphBuilder::GetDeviceList(0);

	CString videoDev, audioDev;
	int nsel = ((CComboBox*)GetDlgItem(IDC_VIDEO_DEVICE_LIST))->GetCurSel();
	((CComboBox*)GetDlgItem(IDC_VIDEO_DEVICE_LIST))->GetLBText(nsel, videoDev);
	nsel = ((CComboBox*)GetDlgItem(IDC_AUDIO_DEVICE_LIST))->GetCurSel();
	((CComboBox*)GetDlgItem(IDC_AUDIO_DEVICE_LIST))->GetLBText(nsel, audioDev);


	_tstring deviceName = _T("video=");
	deviceName += m_videodecs.front();
	deviceName += _T(":audio=");
	deviceName += m_audiodecs.front();

	ISuperSettings * theSetting = m_AVController->GetSettingInterface();
	BSTR dev = SysAllocString(deviceName.c_str());

	// ## THIS IS DEFAULT SETTING
	//HRESULT res = theSetting->SetSourceDevice(dev, _T("-fmt:video_size=640x480;framerate=30$-codeca:sample_rate=44100;channel_layout=3;"));
	// 调试向服务器推流的失败问题
	//HRESULT res = theSetting->SetSourceDevice(dev, \
		_T("-fmt:video_size=640x480;framerate=30$-codeca:sample_rate=44100;channel_layout=3;"));
	
	//HRESULT res = theSetting->SetSourceDevice(dev, \
		_T("-fmt:video_size=640x480;framerate=30$-codeca:sample_rate=44100"));
	//HRESULT res = theSetting->SetSourceDevice(_T("video=HD WebCam:audio=麦克风 (Realtek High Definition Audio)"), _T("-fmt:video_size=640x360;framerate=15$-codeca:sample_rate=44100;channel_layout=3;"));
	//HRESULT res = theSetting->SetSourceDevice(_T("video=Integrated Camera:audio=Conexant 20672 SmartAudio HD"), _T("-codeca:sample_rate=44100;channel_layout=3;"));
	//HRESULT res = S_OK;

	HRESULT res = theSetting->SetSourceDevice(dev, \
		//_T("-fmt:re;rtbufsize=80000000;thread_queue_size=32;pix_fmt=yuv420p;profile=main;preset=ultrafast;tune=zerolatency;video_size=640x480;framerate=30$-codeca:sample_rate=44100;channel_layout=3;"));
		_T("-fmt:video_size=640x480;framerate=30;rtbufsize=80000000;$-codeca:sample_rate=44100;channel_layout=3;"));
	SysFreeString(dev);
	if (res != S_OK)
		return;

	CString strDate;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	strDate.Format(_T("\\%d_%d_%d__%d_%d_%d__%d.mp4"), 
		sysTime.wYear, sysTime.wMonth, sysTime.wDay, 
		sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
	savePath.Append(strDate);
	

    //theSetting->SetRTMPServer(_T("rtmp://localhost:1935/test/mystream"), _T("codecv:b=1000000;g=90;$codeca:b=64000"));
	// 根据是否录像，应使用不同的参数，如果录像就使用NULL，以防设置的参数不一致
	theSetting->SetRTMPServer(::SysAllocString(OS::StringA2T(m_rtmpUrl.c_str()).c_str()), \
		//NULL);
		_T("-fmt:live=-1;rtbufsize=80000000;$-codecv:b=1000000;g=90;$-codeca:b=64000;"));


	BSTR bstrSavePath = savePath.AllocSysString(); 

	theSetting->SetRecordPath(bstrSavePath, _T("-codecv:b=1000000;g=90;$-codeca:b=64000;"));

	SysFreeString(bstrSavePath);

	
		
	theSetting->SetMessageLoopMode(this->m_hWnd, this->GetDlgItem(IDC_TESTVIDEO)->m_hWnd, WM_VIDEO_UPDATE);
	
	theSetting->StarPlay(TRUE);
	
	GetDlgItem(IDC_START_VIDEO)->EnableWindow(FALSE);
	GetDlgItem(IDC_RECORD_VIDEO)->EnableWindow(FALSE);
	GetDlgItem(IDC_STOP_VIDEO)->EnableWindow(TRUE);
	GetDlgItem(IDC_PAUSE_VIDEO)->EnableWindow(TRUE);

	GetDlgItem(IDC_PREVIEW)->EnableWindow(FALSE);
	GetDlgItem(IDC_CONFIG)->EnableWindow(FALSE);

	m_play_status = 1;
	SetDlgItemText(IDC_PAUSE_VIDEO, _T("暂停(&P)"));

	setStartTime();
#ifdef LOGIN_MODE
	requestStartVideo();
	SetTimer(1, 5000, NULL);
#endif
}

void CKaiboCaptureDlg::setStartTime()
{
	m_starttime = getSystemTime(IDC_START_TIME);
	SetDlgItemText(IDC_END_TIME, _T(""));
	SetDlgItemText(IDC_PLAYED_TIME, _T(""));
}

void CKaiboCaptureDlg::setEndTime()
{
	m_endtime = getSystemTime(IDC_END_TIME);
	long elapsed = m_endtime - m_starttime;
	
	CString strTimeElapsed;
	int h = elapsed/3600;
	int m = (elapsed - h * 3600) / 60;
	int s = (elapsed - h * 3600 - m * 60);
	strTimeElapsed.Format(_T("%d小时%d分%d秒"), h, m, s);
	
	SetDlgItemText(IDC_PLAYED_TIME, strTimeElapsed);
}

afx_msg LRESULT CKaiboCaptureDlg::OnVideoUpdate(WPARAM wParam, LPARAM lParam)
{
	if (m_AVController)
	{
		ISuperSettings * theSetting = m_AVController->GetSettingInterface();
		
		theSetting->RenderVideoFrame();
	}
	if (m_previewLocalController)
	{
		ISuperSettings * theSetting = m_previewLocalController->GetSettingInterface();

		theSetting->RenderVideoFrame();
	}
	if (m_previewRtmpController)
	{
		ISuperSettings * theSetting = m_previewRtmpController->GetSettingInterface();

		theSetting->RenderVideoFrame();
	}
	return 0;
}


void CKaiboCaptureDlg::OnBnClickedStopVideo()
{
	// TODO: Add your control notification handler code here
	if (m_AVController)
	{
		ISuperSettings * theSetting = m_AVController->GetSettingInterface();
		theSetting->StopPlay();
	}
	 
	GetDlgItem(IDC_START_VIDEO)->EnableWindow(TRUE);
	GetDlgItem(IDC_STOP_VIDEO)->EnableWindow(FALSE);
	GetDlgItem(IDC_RECORD_VIDEO)->EnableWindow(TRUE);
	GetDlgItem(IDC_PAUSE_VIDEO)->EnableWindow(FALSE);
	GetDlgItem(IDC_PREVIEW)->EnableWindow(TRUE);

	GetDlgItem(IDC_CONFIG)->EnableWindow(TRUE);
	
	setEndTime();

#ifdef LOGION_MODE
	if (m_play_status == 1) {
		requestEndVideo();
		KillTimer(1);
	}
#endif

	m_play_status = 0;
	m_record_status = 0;
	m_preview_status = 0;
	// test permission
	//requestVideoPermission();
}
void CKaiboCaptureDlg::StopLocalPreview()
{
	if (m_previewLocalController) {
		ISuperSettings * theSetting = m_previewLocalController->GetSettingInterface();
		theSetting->StopPlay();
	}
}
void CKaiboCaptureDlg::StopRtmpPreview()
{
	if (m_previewRtmpController) {
		ISuperSettings * theSetting = m_previewRtmpController->GetSettingInterface();
		theSetting->StopPlay();
	}
}
void CKaiboCaptureDlg::StopPreview()
{
	if (m_previewLocalController) {
		ISuperSettings * theSetting = m_previewLocalController->GetSettingInterface();
		theSetting->StopPlay();
	}
	if (m_previewRtmpController) {
		ISuperSettings * theSetting = m_previewRtmpController->GetSettingInterface();
		if (1 == m_connectRtmpStatus)
			theSetting->StopPlay();
	}

	m_preview_status = 0;
}


void CKaiboCaptureDlg::OnStnClickedTestvideo()
{
	// TODO:  在此添加控件通知处理程序代码
}


void CKaiboCaptureDlg::OnBnClickedOk()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_AVController) {
		ISuperSettings * theSetting = m_AVController->GetSettingInterface();

		if (0 == m_play_status) {
			theSetting->StopPlay();
			m_play_status = 0;
		}
	}
	CDialogEx::OnOK();
}


void CKaiboCaptureDlg::OnBnClickedVideoSavePath()
{
	// TODO: Add your control notification handler code here
	char szPath[MAX_PATH]; //存放选择的目录路径 
	CString str;

	ZeroMemory(szPath, sizeof(szPath));

	BROWSEINFO bi;
	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = (LPWSTR)szPath;
	bi.lpszTitle = _T("请选择需要打开的目录：");
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;
	//弹出选择目录对话框
	LPITEMIDLIST lp = SHBrowseForFolder(&bi);

	if (lp && SHGetPathFromIDList(lp, (LPWSTR)szPath)) {
		str.Format(_T("%s"), szPath);
		SetDlgItemText(IDC_EDIT_VIDEO_SAVE_PATH, str);
	}
}


void CKaiboCaptureDlg::OnBnClickedPauseVideo()
{
	if (m_pause_status == 0) {
		m_pause_status = 1;
		SetDlgItemText(IDC_PAUSE_VIDEO, _T("继续(&P)"));
		if (m_AVController) {
			ISuperSettings * theSetting = m_AVController->GetSettingInterface();

			theSetting->PausePlay();
		}
	} else if (m_pause_status == 1) {
		m_pause_status = 0;
		SetDlgItemText(IDC_PAUSE_VIDEO, _T("暂停(&P)"));
		if (m_AVController) {
			ISuperSettings * theSetting = m_AVController->GetSettingInterface();

			theSetting->ResumePlay();
		}
	}
}
/******************************************************************************************************************************
* Function    : preview
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/23 15:20:14
* Modify Date : 2016/03/23 15:20:14
******************************************************************************************************************************/
HRESULT CKaiboCaptureDlg::previewLocal(HWND hwnd)
{
	//std::list<_tstring> audiodecs = CGraphBuilder::GetDeviceList(1);
	//std::list<_tstring> videodecs = CGraphBuilder::GetDeviceList(0);
	if (m_previewLocalController == NULL)
		m_previewLocalController = new CPreviewLocalFilters();

	_tstring deviceName = _T("video=");
	deviceName += m_videodecs.front();
	deviceName += _T(":audio=");
	deviceName += m_audiodecs.front();

	ISuperSettings * theSetting = m_previewLocalController->GetSettingInterface();
	BSTR dev = SysAllocString(deviceName.c_str());

	HRESULT res = theSetting->SetSourceDevice(dev, \
		_T("-fmt:video_size=640x480;framerate=30$-codeca:sample_rate=44100;channel_layout=3;"));
	if (res != S_OK) {
		printf("can not connect %s\n", m_rtmpUrl.c_str());
		return res;
	}
	SysFreeString(dev);
	if (res != S_OK)
		return -1;

	theSetting->SetRTMPServer(::SysAllocString(OS::StringA2T(m_rtmpUrl.c_str()).c_str()), NULL);
	theSetting->SetMessageLoopMode(this->m_hWnd, hwnd, WM_LOCAL_PREVIEW);

	theSetting->StarPlay(TRUE);

	return 0;
}
/******************************************************************************************************************************
* Function    : previewRtmpStream
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/23 15:29:52
* Modify Date : 2016/03/23 15:29:52
******************************************************************************************************************************/
HRESULT CKaiboCaptureDlg::previewRtmpStream(HWND hwnd)
{
	if (m_previewRtmpController == NULL)
		m_previewRtmpController = new CPreviewRtmpFilters();

	_tstring fileName = OS::StringA2T(m_rtmpUrl.c_str()).c_str();
	ISuperSettings * theSetting = m_previewRtmpController->GetSettingInterface();
	BSTR dev = SysAllocString(fileName.c_str());

	int i = 0;
	int WAITTIMES = 300;
	if (2 == m_connectRtmpStatus) {
		while (2 == m_connectRtmpStatus && i <= WAITTIMES) {
			printf("################ status: %d\n", m_connectRtmpStatus);
			Sleep(100);
			i++;
		}

		if (m_previewRtmpController) {
			ISuperSettings * theSetting = m_previewRtmpController->GetSettingInterface();
			if (1 == m_connectRtmpStatus)
				theSetting->StopPlay();
			if (i >= WAITTIMES) {
				return 0;
			}
		}
	}

	m_connectRtmpStatus = 2; // connecting...
	HRESULT res = theSetting->SetSourceFileName(SysAllocString(OS::StringA2T(m_rtmpUrl.c_str()).c_str()));
	//HRESULT res = theSetting->SetSourceFileName(SysAllocString(_T("rtmp://192.168.177.112:1935/live/stream1")));
	//HRESULT res = theSetting->SetSourceFileName(SysAllocString(_T("rtmp://localhost:1935/live/stream1")));
	if (res != S_OK) {
		printf("can not connect %s\n", m_rtmpUrl.c_str());
		return res;
	}

	SysFreeString(dev);
	if (res != S_OK)
		return -1;

	theSetting->SetRTMPServer(_T("NONE"), NULL);
	theSetting->SetMessageLoopMode(this->m_hWnd, hwnd, WM_RTMP_PREVIEW);

	theSetting->StarPlay(TRUE);
	m_connectRtmpStatus = 1; // establised.

	return 0;
}

void CKaiboCaptureDlg::OnBnClickedPreview()
{
	// 启动预览，测试视音频功能
	CPreviewDlg dlg;
	dlg.DoModal();
}
void CKaiboCaptureDlg::OnBnClickedMediaSwitch()
{
	// 根据媒体类型，予以切换
	char szPath[MAX_PATH];
	CString str;

	ZeroMemory(szPath, sizeof(szPath));

	CFileDialog fileDlg(TRUE,
		_T(".mp4"),
		_T(""), // 默认打开的文件名 
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR,
		_T("视频文件(*.mp4)|*.mp4|所有文件(*.*) |*.*||"));

	fileDlg.m_ofn.lpstrInitialDir = _T("d:\\");//初始化路径。
	if (fileDlg.DoModal() == IDOK) {
		str = fileDlg.GetPathName();
	}
	if (str.GetLength() == 0) {
		AfxMessageBox(_T("Media file invalid."));
		return;
	}

	switchMedia(str);
}
HRESULT CKaiboCaptureDlg::switchMedia(CString str)
{
	if (m_AVController == NULL)
		m_AVController = new CXiaozaiFilters();

	ISuperSettings * theSetting = m_AVController->GetSettingInterface();
	HRESULT res = theSetting->SetSourceDevice(SysAllocString(str), NULL);

	if (res != S_OK)
		return E_FAIL;
	theSetting->SetMessageLoopMode(this->m_hWnd, this->GetDlgItem(IDC_TESTVIDEO)->m_hWnd, WM_VIDEO_UPDATE);
	theSetting->StarPlay(TRUE);

	GetDlgItem(IDC_START_VIDEO)->EnableWindow(FALSE);
	GetDlgItem(IDC_RECORD_VIDEO)->EnableWindow(FALSE);
	GetDlgItem(IDC_STOP_VIDEO)->EnableWindow(TRUE);
	GetDlgItem(IDC_PAUSE_VIDEO)->EnableWindow(TRUE);
	GetDlgItem(IDC_PREVIEW)->EnableWindow(FALSE);
	GetDlgItem(IDC_CONFIG)->EnableWindow(FALSE);

	SetDlgItemText(IDC_PAUSE_VIDEO, _T("暂停(&P)"));

	return S_OK;
}
/******************************************************************************************************************************
* Function    : 
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/16 16:51:49
* Modify Date : 2016/03/16 16:51:49
******************************************************************************************************************************/
long CKaiboCaptureDlg::getSystemTime(int idc)
{
	time_t t = time(0);
	struct tm *p;
	p = localtime(&t);
	char s[80];
	strftime(s, 80, "%Y-%m-%d %H:%M:%S", p);
	SetDlgItemText(idc, OS::StringA2T(s).c_str());
	printf("%d: %s\n", (int)t, s);

	return (long)t;
}
/******************************************************************************************************************************
* Function    : 
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/17 11:09:43
* Modify Date : 2016/03/17 11:09:43
******************************************************************************************************************************/
static size_t process_data(void *data, size_t size, size_t nmemb, std::string &content)
{
	long sizes = size * nmemb;
	std::string temp;
	temp = std::string((char*)data, sizes);
	content += temp;
	return sizes;
}
/******************************************************************************************************************************
* Function    : 
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/17 11:09:49
* Modify Date : 2016/03/17 11:09:49
******************************************************************************************************************************/
int CKaiboCaptureDlg::initCurl(struct curl_slist*& http_header, CURL*& curl, std::string& content)
{
	int ret;
	curl = curl_easy_init();
	if (curl == NULL)
	{
		ret = -1;
		printf("curl_easy_init failed.\n");
		return ret;
	}

	http_header = curl_slist_append(http_header, "Accept: */*");
	http_header = curl_slist_append(http_header, "Charset: utf-8");

	content.clear();
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &process_data);
	curl_easy_setopt(curl, CURLOPT_HEADER, 0);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_header);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 2000);
	curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

	return 0;
}
/******************************************************************************************************************************
* Function    : 
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/17 11:09:58
* Modify Date : 2016/03/17 11:09:58
******************************************************************************************************************************/
int CKaiboCaptureDlg::setCurlPost(CURL *curl, const char* posturl, const char* postfields)
{
	int ret;
	ret = curl_easy_setopt(curl, CURLOPT_URL, posturl);
	if (ret > 0) {
		printf("url: %s error occurred.\n", posturl);
		return -1;
	}
	ret = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
	if (ret > 0) {
		printf("field: %s options error occurred.\n", postfields);
		return -1;
	}
	return 0;
}
/******************************************************************************************************************************
* Function    : 
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/17 11:10:03
* Modify Date : 2016/03/17 11:10:03
******************************************************************************************************************************/
int CKaiboCaptureDlg::performCurl(CURL* curl, std::string& content)
{
	int res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("curl_easy_perform failed.\n");
		return -1;
	}

	long retcode = 0;
	CURLcode code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retcode);
	if ((code == CURLE_OK) && retcode == 200) {
		printf("Request success.\n");
		printf("%s\n", content.c_str());
		return 0;
	}
	else {
		printf("Request from server failed.\n");
		return -3;
	}
}
/******************************************************************************************************************************
* Function    : 
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/17 11:10:13
* Modify Date : 2016/03/17 11:10:13
******************************************************************************************************************************/
void CKaiboCaptureDlg::finalizeCurl(struct curl_slist *http_header, CURL *curl)
{
	curl_slist_free_all(http_header);
	curl_easy_cleanup(curl);
}
/******************************************************************************************************************************
* Function    : 
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/17 11:10:19
* Modify Date : 2016/03/17 11:10:19
******************************************************************************************************************************/
int CKaiboCaptureDlg::requestStartVideo()
{
	// 连接应用服务器
	char posturl[2048];
	char postfields[1024];

	sprintf(posturl, POSTURL_START_VIDEO, m_servAddress.c_str(), m_servPort);

	CString strName;
	GetDlgItemText(IDC_VIDEOCAST_NAME, strName);
	CString strPass;
	GetDlgItemText(IDC_CRYPT_KEY, strPass);

	m_allow_comment = ((CButton*)GetDlgItem(IDC_ALLOW_COMMENT))->GetCheck();
	m_video_encrypt = ((CButton*)GetDlgItem(IDC_VIDEO_ENCRYPT))->GetCheck();

	size_t size = strName.GetLength();
	std::string pMsg = OS::StringT2A(strName.GetBuffer(0));
	printf("video name: %s\n", utf82UnicodeString(pMsg.c_str(), &size));

	size = strPass.GetLength();
	std::string password = OS::StringT2A(strPass.GetBuffer(0));

	strName.ReleaseBuffer();
	strPass.ReleaseBuffer();

	// 直播名称可能要转换为UTF8S
	if (m_video_encrypt) {
		sprintf(postfields, POSTFIELDS_START_ENCRYPTVIDEO, \
			m_videocastid.c_str(), \
			pMsg.c_str(), \
			m_starttime * 1000, \
			m_allow_comment, \
			password.c_str());
	} else {
		sprintf(postfields, POSTFIELDS_START_PLAINVIDEO, \
			m_videocastid.c_str(), \
			pMsg.c_str(), \
			m_starttime * 1000, \
			m_allow_comment);
	}
	printf("Request %s?%s\n", posturl, postfields);
	
	int ret = 0;
	CURL* curl = NULL;
	struct curl_slist* http_header = NULL;
	std::string content;
	initCurl(http_header, curl, content);
	setCurlPost(curl, posturl, postfields);

	ret = performCurl(curl, content);
	if (ret < 0) {
		AfxMessageBox(_T("无法连接应用服务器"));
		return -1;
	}

	finalizeCurl(http_header, curl);

	int result = parseJsonResponse(content);
	if (1 == result) {
		setRtmpUrl();
		//theApp->m_rtmpUrl = url;
		//m_rtmpUrl = "rtmp://192.168.177.112:1935/live/stream2";
		//m_rtmpUrl = "rtmp://localhost:1935/live/stream1";
	} else {
		AfxMessageBox(_T("请求rtmp服务器地址失败"));
		return -1;
	}

	return 0;
}
/******************************************************************************************************************************
* Function    : 
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/17 11:10:26
* Modify Date : 2016/03/17 11:10:26
******************************************************************************************************************************/
int CKaiboCaptureDlg::requestEndVideo()
{
	// 连接应用服务器
	char posturl[2048];
	char postfields[1024];

	sprintf(posturl, POSTURL_END_VIDEO, m_servAddress.c_str(), m_servPort);
	sprintf(postfields, POSTFIELDS_END_VIDEO, \
		m_videocastid.c_str(), \
		m_endtime * 1000);

	printf("Request %s?%s\n", posturl, postfields);

	int ret = 0;
	CURL* curl = NULL;
	struct curl_slist* http_header = NULL;
	std::string content;
	initCurl(http_header, curl, content);
	setCurlPost(curl, posturl, postfields);

	ret = performCurl(curl, content);
	if (ret < 0) {
		AfxMessageBox(_T("无法连接应用服务器"));
		return -1;
	}

	finalizeCurl(http_header, curl);

	int result = parseJsonResponse(content);
	if (1 == result) {
		
	}
	else {
		AfxMessageBox(_T("请求rtmp服务器地址失败"));
		return -1;
	}

	return 0;
}
/******************************************************************************************************************************
* Function    : 
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/17 11:10:31
* Modify Date : 2016/03/17 11:10:31
******************************************************************************************************************************/
int CKaiboCaptureDlg::requestVideoPermission()
{
	// 连接应用服务器
	char posturl[2048];
	char postfields[1024];

	sprintf(posturl, POSTURL_VIDEO_PERMISSION, m_servAddress.c_str(), m_servPort);
	sprintf(postfields, POSTFIELDS_VIDEO_PERMISSION, m_videocastid.c_str());

	printf("Request %s?%s\n", posturl, postfields);

	int ret = 0;
	CURL* curl = NULL;
	struct curl_slist* http_header = NULL;
	std::string content;
	initCurl(http_header, curl, content);
	setCurlPost(curl, posturl, postfields);

	ret = performCurl(curl, content);
	if (ret < 0) {
		AfxMessageBox(_T("无法连接应用服务器"));
		return -1;
	}

	finalizeCurl(http_header, curl);

	int result = parseJsonResponse(content);
	if (1 == result) {
		
	} else {
		AfxMessageBox(_T("请求rtmp服务器地址失败"));
		return -1;
	}
	return 0;
}
/******************************************************************************************************************************
* Function    : parseJsonResponse
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/18 11:41:33
* Modify Date : 2016/03/18 11:41:33
******************************************************************************************************************************/
int CKaiboCaptureDlg::parseJsonResponse(std::string content)
{
	Json::Reader reader;
	Json::Value root;
	int icode = -1;

	if (reader.parse(content, root)) {
		std::string	code = root["code"].asString();
		icode = atoi(code.c_str());

		if (icode < 0) {
			printf("code is error.\n");
			return -1;
		}

		std::string	msg = root["message"].asString();
		size_t size = msg.size();
		std::string pMsg = utf82UnicodeString(msg.c_str(), &size);

		if (APPSERVER_RESPONSE_CODE_SUCCESS == icode) {
			std::string result = root["datas"]["isResult"].asString();
			int ires = atoi(result.c_str());
			
			if (1 == ires) {
				printf("result: %d, id: %s\n", ires, m_videocastid.c_str());
				return ires;
			} else {
				char buff[512] = { 0 };
				sprintf(buff, "%s", pMsg.c_str());
				std::cout << pMsg << std::endl;

				CString str(buff);
				AfxMessageBox(str);
				return -2;
			}
		} else {
			if (m_AVController) {
				ISuperSettings * theSetting = m_AVController->GetSettingInterface();
				theSetting->StopPlay();
			}
			char buff[512] = { 0 };
			sprintf(buff, "%s", pMsg.c_str());
			std::cout << pMsg << std::endl;
			CString str(buff);
			AfxMessageBox(str);
			return -3;
		}
	}

	return icode;
}





void CKaiboCaptureDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case 1:
		//做该做的事情
#ifdef LOGIN_MODE
		requestVideoPermission();
#endif
		//当不需要的时候在此处调用KillTimer(1); 
		break;
	default:
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CKaiboCaptureDlg::OnBnClickedRecordVideo()
{
	CString savePath;
	GetDlgItemText(IDC_EDIT_VIDEO_SAVE_PATH, savePath);
	if (savePath.IsEmpty()) {
		AfxMessageBox(_T("请选择录像存储路径"));
		return;
	}

	CString videocastName;
	GetDlgItemText(IDC_VIDEOCAST_NAME, videocastName);
	if (videocastName.IsEmpty()) {
		AfxMessageBox(_T("请指定直播视频题目"));
		return;
	}

	// TODO: Add your control notification handler code here
	std::list<_tstring> audiodecs = CGraphBuilder::GetDeviceList(1);
	std::list<_tstring> videodecs = CGraphBuilder::GetDeviceList(0);
	if (m_AVController == NULL)
		m_AVController = new CXiaozaiFilters();
	//m_AVController->Test(this->GetDlgItem(IDC_TESTVIDEO)->m_hWnd);

	_tstring deviceName = _T("video=");
	deviceName += videodecs.front();
	deviceName += _T(":audio=");
	deviceName += audiodecs.front();

	ISuperSettings * theSetting = m_AVController->GetSettingInterface();
	BSTR dev = SysAllocString(deviceName.c_str());

	HRESULT res = theSetting->SetSourceDevice(dev, \
		_T("-fmt:video_size=640x480;framerate=30$-codeca:sample_rate=44100;channel_layout=3;"));

	SysFreeString(dev);
	if (res != S_OK)
		return;

	CString strDate;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	strDate.Format(_T("\\%d_%d_%d__%d_%d_%d__%d.mp4"),
		sysTime.wYear, sysTime.wMonth, sysTime.wDay,
		sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
	savePath.Append(strDate);
	BSTR bstrSavePath = savePath.AllocSysString();

	theSetting->SetRecordPath(bstrSavePath, _T("-codecv:b=1000000;g=90;$-codeca:b=64000;"));
	SysFreeString(bstrSavePath);
	theSetting->SetMessageLoopMode(this->m_hWnd, this->GetDlgItem(IDC_TESTVIDEO)->m_hWnd, WM_VIDEO_UPDATE);
	theSetting->StarPlay(TRUE);

	GetDlgItem(IDC_START_VIDEO)->EnableWindow(FALSE);
	GetDlgItem(IDC_RECORD_VIDEO)->EnableWindow(FALSE);
	GetDlgItem(IDC_STOP_VIDEO)->EnableWindow(TRUE);
	GetDlgItem(IDC_PAUSE_VIDEO)->EnableWindow(TRUE);
	GetDlgItem(IDC_PREVIEW)->EnableWindow(FALSE);
	GetDlgItem(IDC_CONFIG)->EnableWindow(FALSE);

	m_play_status = 1;
	SetDlgItemText(IDC_PAUSE_VIDEO, _T("暂停(&P)"));

	setStartTime();
	//requestStartVideo();
	//SetTimer(1, 5000, NULL);
}


void CKaiboCaptureDlg::OnBnClickedConfig()
{
	CXzConfig xzConfig;
	xzConfig.DoModal();
}


void CKaiboCaptureDlg::OnBnClickedVideoEncrypt()
{
	m_video_encrypt = ((CButton*)GetDlgItem(IDC_VIDEO_ENCRYPT))->GetCheck();
	switch (m_video_encrypt) {
	case 0:
		GetDlgItem(IDC_CRYPT_KEY)->EnableWindow(FALSE);
		break;
	case 1:
		GetDlgItem(IDC_CRYPT_KEY)->EnableWindow(TRUE);
		break;
	default:
		GetDlgItem(IDC_CRYPT_KEY)->EnableWindow(FALSE);
	}
}



void CKaiboCaptureDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	CSliderCtrl*	pSCVolume	= (CSliderCtrl*)GetDlgItem(IDC_SLD_VOLUME);
	CSliderCtrl*	pSCMicphone	= (CSliderCtrl*)GetDlgItem(IDC_SLD_MICPHONE);

	CAudioVolume cav;
	if ((CSliderCtrl*)pScrollBar == pSCVolume) {
		int val = pSCVolume->GetPos();
		cav.SetSystemVolume(val * 0.01);
	} else if ((CSliderCtrl*)pScrollBar == pSCMicphone) {
		int val = pSCMicphone->GetPos();
		cav.SetSystemMicVolume(val * 0.01);
	}
	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}
