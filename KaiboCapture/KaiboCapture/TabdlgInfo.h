#pragma once


// CTabdlgInfo dialog

class CTabdlgInfo : public CDialogEx
{
	DECLARE_DYNAMIC(CTabdlgInfo)

public:
	CTabdlgInfo(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTabdlgInfo();

// Dialog Data
	enum { IDD = IDD_TABDLG_INFO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
