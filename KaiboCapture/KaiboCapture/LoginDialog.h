#pragma once

#include <string>
#include "curl/curl.h"

// CLoginDialog dialog

class CLoginDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CLoginDialog)

public:
	CLoginDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLoginDialog();


public:
	int loadConfig();
	int requestLogin();
	CString getUsername() {
		return m_username;
	}
	CString getPassword() {
		return m_password;
	}
	std::string getVideocastId() {
		return m_videocastid;
	}
	int getLoginStatus() {
		return m_loginok;
	}
	std::string parseJsonResponse(std::string content);
	TCHAR* utf82Unicode(const char* utf, size_t *unicode_number);

	int savePasswd();

private:
	CString m_username;
	CString m_password;

	int m_loginok;

	int initCurl(struct curl_slist*& http_header, CURL*& curl, std::string& content);
	int setCurlPost(CURL* curl, const char* posturl, const char* postfields);
	int performCurl(CURL* curl, std::string& content);
	void finalizeCurl(struct curl_slist *http_header, CURL *curl);

	char* hexstr(unsigned char *buf, int len);

	std::string			m_videocastid;
// Dialog Data
	enum { IDD = IDD_LOGIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
};
