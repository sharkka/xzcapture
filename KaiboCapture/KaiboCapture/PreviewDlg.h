#pragma once
#include "KaiboCaptureDlg.h"

// CPreviewDlg dialog

class CPreviewDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPreviewDlg)

private:
	HANDLE m_hLocalThread;
	HANDLE m_hRtmpThread;
public:
	CPreviewDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPreviewDlg();

	HWND m_pLocalWnd;
	HWND m_pRtmpWnd;
	CKaiboCaptureDlg* m_parentWnd;

// Dialog Data
	enum { IDD = IDD_DLG_PREVIEW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
