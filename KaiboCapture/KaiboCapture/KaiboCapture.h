
// KaiboCapture.h : PROJECT_NAME ���ó�ʽ����Ҫ���^�n
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�� PCH �����˙n��ǰ�Ȱ��� 'stdafx.h'"
#endif

#include "resource.h"		// ��Ҫ��̖
#include <string>


// CKaiboCaptureApp: 
// Ո��醌�����e�� KaiboCapture.cpp
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
// ����
public:
	virtual BOOL InitInstance();

// ��ʽ�a����
	void Test1();

	DECLARE_MESSAGE_MAP()

};

extern CKaiboCaptureApp theApp;