// XzConfig.cpp : implementation file
//

#include "stdafx.h"
#include "KaiboCapture.h"
#include "XzConfig.h"
#include "afxdialogex.h"
#include "KaiboCaptureDlg.h"


// CXzConfig dialog

IMPLEMENT_DYNAMIC(CXzConfig, CDialogEx)

CXzConfig::CXzConfig(CWnd* pParent /*=NULL*/)
	: CDialogEx(CXzConfig::IDD, pParent)
{

}

CXzConfig::~CXzConfig()
{
}

void CXzConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_CONFIG, m_configTab);
}


BEGIN_MESSAGE_MAP(CXzConfig, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_CONFIG, &CXzConfig::OnTcnSelchangeTabConfig)
	ON_BN_CLICKED(IDOK, &CXzConfig::OnBnClickedOk)
END_MESSAGE_MAP()


// CXzConfig message handlers


BOOL CXzConfig::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CDialog::OnInitDialog();
	m_configTab.InsertItem(0, _T("信息"));
	m_configTab.InsertItem(1, _T("设备"));
	m_configTab.InsertItem(2, _T("关于"));

	m_infoTab.Create(IDD_TABDLG_INFO, GetDlgItem(IDC_TAB_CONFIG));
	m_deviceTab.Create(IDD_TABDLG_DEVICE, GetDlgItem(IDC_TAB_CONFIG));
	m_aboutTab.Create(IDD_TABDLG_ABOUT, GetDlgItem(IDC_TAB_CONFIG));

	CKaiboCaptureDlg* pwnd = (CKaiboCaptureDlg*)GetParent();
	CWnd* videoDlg = m_deviceTab.GetDlgItem(IDC_CAMERA_LIST);
	CWnd* audioDlg = m_deviceTab.GetDlgItem(IDC_MICPHONE_LIST);

	pwnd->setVideoDeviceList(videoDlg);
	pwnd->setAudioDeviceList(audioDlg);

	CRect rs;
	m_configTab.GetClientRect(&rs);

	//调整子对话框在父窗口中的位置，根据实际修改  
	rs.top += 25;
	rs.bottom -= 3;
	rs.left += 1;
	rs.right -= 4;
 
	m_infoTab.MoveWindow(&rs);
	m_deviceTab.MoveWindow(&rs);
	m_aboutTab.MoveWindow(&rs);

	m_infoTab.ShowWindow(true);
	m_deviceTab.ShowWindow(true);
	m_aboutTab.ShowWindow(true);

	m_configTab.SetCurSel(1);
	m_infoTab.ShowWindow(false);
	m_deviceTab.ShowWindow(true);
	m_aboutTab.ShowWindow(false);

	return TRUE;
}

void CXzConfig::OnTcnSelchangeTabConfig(NMHDR *pNMHDR, LRESULT *pResult)
{
	int CurSel = m_configTab.GetCurSel();
	switch (CurSel) {
	case 0:
		m_infoTab.ShowWindow(true);
		m_deviceTab.ShowWindow(false);
		m_aboutTab.ShowWindow(false);
		break;
	case 1:
		m_infoTab.ShowWindow(false);
		m_deviceTab.ShowWindow(true);
		m_aboutTab.ShowWindow(false);
		break;
	case 2:
		m_infoTab.ShowWindow(false);
		m_deviceTab.ShowWindow(false);
		m_aboutTab.ShowWindow(true);
		break;
	default:
		;
	}
	*pResult = 0;
}


void CXzConfig::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CString videoDev;
	m_deviceTab.GetDlgItemText(IDC_CAMERA_LIST, videoDev);
	CString audioDev;
	m_deviceTab.GetDlgItemText(IDC_MICPHONE_LIST, audioDev);

	wprintf(_T("%s, %s\n"), videoDev.GetBuffer(0), audioDev.GetBuffer(0));
	CDialogEx::OnOK();
}
