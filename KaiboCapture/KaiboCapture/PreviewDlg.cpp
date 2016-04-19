// PreviewDlg.cpp : implementation file
//

#include "stdafx.h"
#include "KaiboCapture.h"
#include "KaiboCaptureDlg.h"
#include "PreviewDlg.h"
#include "afxdialogex.h"
#include "AudioVolume.h"


// CPreviewDlg dialog

IMPLEMENT_DYNAMIC(CPreviewDlg, CDialogEx)

CPreviewDlg::CPreviewDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPreviewDlg::IDD, pParent)
{

}

CPreviewDlg::~CPreviewDlg()
{
}

void CPreviewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPreviewDlg, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &CPreviewDlg::OnBnClickedCancel)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CPreviewDlg message handlers

unsigned int __stdcall proc_playrtmp(void* param)
{
	CPreviewDlg* dlg = (CPreviewDlg*)param;
	HWND rtmpWnd = dlg->GetDlgItem(IDC_VIDEO_RTMP)->GetSafeHwnd();
	CKaiboCaptureDlg* pwnd = (CKaiboCaptureDlg*)dlg->m_parentWnd;
	HRESULT res = pwnd->previewRtmpStream(rtmpWnd);
	if (res != S_OK) {
		dlg->SetDlgItemText(IDC_VIDEO_RTMP, _T("无法连接远程流媒体服务器"));
		// write log message here
	}
	return 0;
}
unsigned int __stdcall proc_playlocal(void* param)
{
	CPreviewDlg* dlg = (CPreviewDlg*)param;
	HWND localWnd = dlg->GetDlgItem(IDC_VIDEO_LOCAL)->GetSafeHwnd();
	CKaiboCaptureDlg* pwnd = (CKaiboCaptureDlg*)dlg->m_parentWnd;
	HRESULT res = pwnd->previewLocal(localWnd);
	if (res != S_OK) {
		dlg->SetDlgItemText(IDC_VIDEO_LOCAL, _T("无法打开本地媒体设备"));
		// write log message here
	}
	return 0;
}

BOOL CPreviewDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_parentWnd = (CKaiboCaptureDlg*)GetParent();

	//HANDLE hThread;
	unsigned int threadId;
	m_hLocalThread = (HANDLE)_beginthreadex(NULL, 0, proc_playlocal, this, NULL, &threadId);
	SetDlgItemText(IDC_VIDEO_LOCAL, _T("正在打开本地媒体设备。。。"));
	//CloseHandle(hThread);
	//WaitForSingleObject(hThread, INFINITE);

	m_hRtmpThread = (HANDLE)_beginthreadex(NULL, 0, proc_playrtmp, this, NULL, &threadId);
	SetDlgItemText(IDC_VIDEO_RTMP, _T("正在连接远程流媒体服务器。。。"));
	//CloseHandle(hThread);

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

	return TRUE;
}


void CPreviewDlg::OnBnClickedCancel()
{
	CKaiboCaptureDlg* pwnd = (CKaiboCaptureDlg*)GetParent();

	WaitForSingleObject(m_hLocalThread, 2000);
	CloseHandle(m_hLocalThread);
	pwnd->StopLocalPreview();

	WaitForSingleObject(m_hRtmpThread, 2000);
	CloseHandle(m_hRtmpThread);
	pwnd->StopRtmpPreview();
	
	
	//pwnd->StopPreview();
	
	
	CDialogEx::OnCancel();
}
/*
// 不使用线程
BOOL CPreviewDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_parentWnd = (CKaiboCaptureDlg*)GetParent();

	HWND localWnd = GetDlgItem(IDC_VIDEO_LOCAL)->GetSafeHwnd();
	CKaiboCaptureDlg* pwnd = (CKaiboCaptureDlg*)m_parentWnd;
	HRESULT res = pwnd->previewLocal(localWnd);
	if (res != S_OK) {
		SetDlgItemText(IDC_VIDEO_LOCAL, _T("无法连接本地媒体设备"));
		// write log message here
	}

	HWND rtmpWnd = GetDlgItem(IDC_VIDEO_RTMP)->GetSafeHwnd();
	res = pwnd->previewRtmpStream(rtmpWnd);
	if (res != S_OK) {
		SetDlgItemText(IDC_VIDEO_RTMP, _T("无法连接远程流媒体服务器"));
		// write log message here
	}

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

	return TRUE;
}


void CPreviewDlg::OnBnClickedCancel()
{
	CKaiboCaptureDlg* pwnd = (CKaiboCaptureDlg*)GetParent();
	pwnd->StopLocalPreview();
	pwnd->StopRtmpPreview();

	CDialogEx::OnCancel();
}
*/
void CPreviewDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	CSliderCtrl*	pSCVolume = (CSliderCtrl*)GetDlgItem(IDC_SLD_VOLUME);
	CSliderCtrl*	pSCMicphone = (CSliderCtrl*)GetDlgItem(IDC_SLD_MICPHONE);

	CAudioVolume cav;
	if ((CSliderCtrl*)pScrollBar == pSCVolume) {
		int val = pSCVolume->GetPos();
		cav.SetSystemVolume(val * 0.01);
	}
	else if ((CSliderCtrl*)pScrollBar == pSCMicphone) {
		int val = pSCMicphone->GetPos();
		cav.SetSystemMicVolume(val * 0.01);
	}
	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}
