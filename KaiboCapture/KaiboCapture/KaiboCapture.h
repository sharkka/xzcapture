
// KaiboCapture.h : PROJECT_NAME 應用程式的主要標頭檔
//

#pragma once

#ifndef __AFXWIN_H__
	#error "對 PCH 包含此檔案前先包含 'stdafx.h'"
#endif

#include "resource.h"		// 主要符號
#include <string>


// CKaiboCaptureApp: 
// 請參閱實作此類別的 KaiboCapture.cpp
//

class CKaiboCaptureApp : public CWinApp
{
public:
	CKaiboCaptureApp();
public:
	std::string			m_servAddress;
	int					m_servPort;
	std::string			m_rtmpUrl;
	std::string			m_username;
	std::string			m_password;
	std::string			m_videocastid;
// 覆寫
public:
	virtual BOOL InitInstance();

// 程式碼實作
	void Test1();

	DECLARE_MESSAGE_MAP()

};

extern CKaiboCaptureApp theApp;