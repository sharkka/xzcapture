
// KaiboCaptureDlg.cpp : 實作檔
//

#include "stdafx.h"
#include "KaiboCapture.h"
#include "KaiboCaptureDlg.h"
#include "afxdialogex.h"
#include "GraphBuilder.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CKaiboCaptureDlg 對話方塊

#define WM_VIDEO_UPDATE   WM_USER+100

CKaiboCaptureDlg::CKaiboCaptureDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CKaiboCaptureDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CKaiboCaptureDlg::~CKaiboCaptureDlg()
{
	if (m_AVController)
		delete m_AVController;
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
	ON_BN_CLICKED(IDC_STOP_VIDEO, &CKaiboCaptureDlg::OnBnClickedStopVideo)
END_MESSAGE_MAP()


// CKaiboCaptureDlg 訊息處理常式

BOOL CKaiboCaptureDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 設定此對話方塊的圖示。當應用程式的主視窗不是對話方塊時，
	// 框架會自動從事此作業
	SetIcon(m_hIcon, TRUE);			// 設定大圖示
	SetIcon(m_hIcon, FALSE);		// 設定小圖示

	// TODO:  在此加入額外的初始設定

	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}

// 如果將最小化按鈕加入您的對話方塊，您需要下列的程式碼，
// 以便繪製圖示。對於使用文件/檢視模式的 MFC 應用程式，
// 框架會自動完成此作業。

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


void CKaiboCaptureDlg::OnBnClickedStartVideo()
{
	// TODO: Add your control notification handler code here
	if (m_AVController == NULL)
		m_AVController = new CXiaozaiFilters();
	//m_AVController->Test(this->GetDlgItem(IDC_TESTVIDEO)->m_hWnd);
	ISuperSettings * theSetting = m_AVController->GetSettingInterface();
	theSetting->SetSourceDevice(_T("video=Integrated Camera:audio=Microphone (Realtek High Definition Audio)"), _T("-fmt:video_size=640x360;framerate=15$-codeca:sample_rate=44100;channel_layout=3;"));
	theSetting->SetRecordPath(_T("d:\\test\\1.mp4"), _T("-codecv:b=1000000;g=90;$-codeca:b=64000;"));
	theSetting->SetMessageLoopMode(this->m_hWnd, this->GetDlgItem(IDC_TESTVIDEO)->m_hWnd, WM_VIDEO_UPDATE);
	theSetting->StarPlay(FALSE);
}


afx_msg LRESULT CKaiboCaptureDlg::OnVideoUpdate(WPARAM wParam, LPARAM lParam)
{
	if (m_AVController)
	{
		ISuperSettings * theSetting = m_AVController->GetSettingInterface();
		
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
}
