#pragma once


// CTabdlgDevice dialog

class CTabdlgDevice : public CDialogEx
{
	DECLARE_DYNAMIC(CTabdlgDevice)

public:
	CTabdlgDevice(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTabdlgDevice();

// Dialog Data
	enum { IDD = IDD_TABDLG_DEVICE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
