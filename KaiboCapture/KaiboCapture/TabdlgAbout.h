#pragma once


// CTabdlgAbout dialog

class CTabdlgAbout : public CDialogEx
{
	DECLARE_DYNAMIC(CTabdlgAbout)

public:
	CTabdlgAbout(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTabdlgAbout();

// Dialog Data
	enum { IDD = IDD_TABDLG_ABOUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
