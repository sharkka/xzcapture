#pragma once

#include "TabdlgInfo.h"
#include "TabdlgDevice.h"
#include "TabdlgAbout.h"
#include "afxcmn.h"

// CXzConfig dialog

class CXzConfig : public CDialogEx
{
	DECLARE_DYNAMIC(CXzConfig)

	CTabdlgInfo		m_infoTab;
	CTabdlgDevice	m_deviceTab;
	CTabdlgAbout	m_aboutTab;
public:
	CXzConfig(CWnd* pParent = NULL);   // standard constructor
	virtual ~CXzConfig();

// Dialog Data
	enum { IDD = IDD_DLG_CONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CTabCtrl m_configTab;
	afx_msg void OnTcnSelchangeTabConfig(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedOk();
};
