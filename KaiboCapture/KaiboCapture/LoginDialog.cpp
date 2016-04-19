/******************************************************************************************************************************
* File Name   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture\LoginDialog.cpp
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* File Base   : LoginDialog
* Brief       : 
* Key Words   :
* File Ext    : cpp
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/14 12:08:57
* MODIFY DATE : 2016/03/14 12:08:57
******************************************************************************************************************************/
#pragma warning(disable:4996)
#include "stdafx.h"
#include "KaiboCapture.h"
#include "LoginDialog.h"
#include "afxdialogex.h"

#include "IniConfig.h"
#include "tstring.h"

#include "curl/curl.h"
#include "json/json.h"
#include "openssl/md5.h"
#include "openssl/evp.h"
#include "openssl/bio.h"
#include "openssl/buffer.h"

//----------------------------------------------------------------------------------------------------------------------------//
// Finally, will be move to a header file
#define POSTURL_LOGIN				"http://%s:%d/xiaozaikaibo/app/secondaryTwo/cuser/login"
#define POSTFIELDS_LOGIN			"username=%s&password=%s"

#define APPSERVER_RESPONSE_CODE_SUCCESS			1000	// 成功
#define APPSERVER_RESPONSE_CODE_NOTALLOWED		1005	// 没有权限

#define CONFIG_FILE_NAME						_T("config.ini")
//----------------------------------------------------------------------------------------------------------------------------//

std::string utf82UnicodeString(const char* utf, size_t *unicode_number);
// CLoginDialog dialog

IMPLEMENT_DYNAMIC(CLoginDialog, CDialogEx)

CLoginDialog::CLoginDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CLoginDialog::IDD, pParent)
{

}

CLoginDialog::~CLoginDialog()
{
}

void CLoginDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}
BEGIN_MESSAGE_MAP(CLoginDialog, CDialogEx)
	ON_BN_CLICKED(IDOK, &CLoginDialog::OnBnClickedOk)
END_MESSAGE_MAP()
/******************************************************************************************************************************
* Function    : Base64Encode
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/04/06 10:44:22
* Modify Date : 2016/04/06 10:44:22
******************************************************************************************************************************/
static char * Base64Encode(const char * input, int length, bool with_new_line)
{
	BIO * bmem = NULL;
	BIO * b64 = NULL;
	BUF_MEM * bptr = NULL;

	b64 = BIO_new(BIO_f_base64());
	if (!with_new_line) {
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	}
	bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, input, length);
	BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);

	char * buff = (char *)malloc(bptr->length + 1);
	memcpy(buff, bptr->data, bptr->length);
	buff[bptr->length] = 0;

	BIO_free_all(b64);

	return buff;
}
/******************************************************************************************************************************
* Function    : Base64Decode
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/04/06 10:44:27
* Modify Date : 2016/04/06 10:44:27
******************************************************************************************************************************/
static char * Base64Decode(char * input, int length, bool with_new_line)
{
	BIO * b64 = NULL;
	BIO * bmem = NULL;
	char * buffer = (char *)malloc(length);
	memset(buffer, 0, length);

	b64 = BIO_new(BIO_f_base64());
	if (!with_new_line) {
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	}
	bmem = BIO_new_mem_buf(input, length);
	bmem = BIO_push(b64, bmem);
	BIO_read(bmem, buffer, length);

	BIO_free_all(bmem);

	return buffer;
}
BOOL StringToWString(const std::string &str, std::wstring &wstr)
{
	int nLen = (int)str.length();
	wstr.resize(nLen, L' ');

	int nResult = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), nLen, (LPWSTR)wstr.c_str(), nLen);

	if (nResult == 0)
	{
		return FALSE;
	}

	return TRUE;
}
//wstring高字节不为0，返回FALSE
BOOL WStringToString(const std::wstring &wstr, std::string &str)
{
	int nLen = (int)(wstr.length() * sizeof(wchar_t));
	str.resize(nLen, ' ');

	int nResult = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wstr.c_str(), nLen, (LPSTR)str.c_str(), nLen, NULL, NULL);

	if (nResult == 0)
	{
		return FALSE;
	}

	return TRUE;
}
/******************************************************************************************************************************
* Function    : 
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/14 12:08:44
* Modify Date : 2016/03/14 12:08:44
******************************************************************************************************************************/
int CLoginDialog::loadConfig()
{
	const TCHAR configFile[] = CONFIG_FILE_NAME;
	
	TCHAR szFilePath[MAX_PATH + 1];
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0;

	std::wstring wfullpath;
	wfullpath.assign(szFilePath);
	wfullpath.append(OS::StringT2W(configFile));

	if (!PathFileExists(wfullpath.c_str())) {
		AfxMessageBox(_T("缺失配置文件"));
		return - 1;
	}

	//Config configSettings(OS::StringT2UTF8(fullpath));
	
	std::string fullpath;
	WStringToString(wfullpath, fullpath);
	Config configSettings(fullpath);

	CKaiboCaptureApp* theApp = (CKaiboCaptureApp*)AfxGetApp();

	theApp->m_servAddress = configSettings.Read("address", theApp->m_servAddress);
	if (theApp->m_servAddress.empty()) {
		AfxMessageBox(_T("应用服务器地址非法"));
		return -1;
	}
	theApp->m_servPort = configSettings.Read("port", theApp->m_servPort);
	if (theApp->m_servPort < 1024 || theApp->m_servPort > 65535) {
		AfxMessageBox(_T("应用服务器端口非法"));
		return -1;
	}
	
	printf("address: %s:%d\n", theApp->m_servAddress.c_str(), theApp->m_servPort);

	std::string strpass = configSettings.Read("password", strpass);
	if (0 == strpass.size())
		SetDlgItemText(IDC_PASSWORD, _T(""));
	else {
		char * dec_output = Base64Decode((char*)strpass.c_str(), strpass.length(), false);
		SetDlgItemText(IDC_PASSWORD, OS::StringA2T(dec_output).c_str());
	}
	int icheck = 0;
	icheck = configSettings.Read("passwdchecked", icheck);
	((CButton*)GetDlgItem(IDC_SAVE_PASSWD))->SetCheck(icheck);

	return 0;
}
/******************************************************************************************************************************
* Function    : process_data
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/16 16:09:04
* Modify Date : 2016/03/16 16:09:04
******************************************************************************************************************************/
static size_t process_data(void *data, size_t size, size_t nmemb, std::string &content)
{
	long sizes = size * nmemb;
	std::string temp;
	temp = std::string((char*)data, sizes);
	content += temp;
	return sizes;
}
/******************************************************************************************************************************
* Function    : initCurl
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/04/06 10:45:47
* Modify Date : 2016/04/06 10:45:47
******************************************************************************************************************************/
int CLoginDialog::initCurl(struct curl_slist*& http_header, CURL*& curl, std::string& content)
{
	int ret;
	curl = curl_easy_init();
	if (curl == NULL)
	{
		ret = -1;
		printf("curl_easy_init failed.\n");
		return ret;
	}

	http_header = curl_slist_append(http_header, "Accept: */*");
	http_header = curl_slist_append(http_header, "Charset: utf-8");

	content.clear();
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &process_data);
	curl_easy_setopt(curl, CURLOPT_HEADER, 0);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
	
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_header);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 5000);
	curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

	return 0;
}
/******************************************************************************************************************************
* Function    : setCurlPost
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/04/06 10:45:59
* Modify Date : 2016/04/06 10:45:59
******************************************************************************************************************************/
int CLoginDialog::setCurlPost(CURL *curl, const char* posturl, const char* postfields)
{
	int ret;
	ret = curl_easy_setopt(curl, CURLOPT_URL, posturl);
	//ret = curl_easy_setopt(curl, CURLOPT_URL, to_utf8string(posturl).c_str());
	if (ret > 0) {
		printf("url: %s error occurred.\n", posturl);
		return -1;
	}
	ret = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
	if (ret > 0) {
		printf("field: %s options error occurred.\n", postfields);
		return -1;
	}
	return 0;
}
/******************************************************************************************************************************
* Function    : performCurl
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/04/06 10:46:12
* Modify Date : 2016/04/06 10:46:12
******************************************************************************************************************************/
int CLoginDialog::performCurl(CURL* curl, std::string& content)
{
	int res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("curl_easy_perform failed.\n");
		return -1;
	}

	long retcode = 0;
	CURLcode code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retcode);
	if ((code == CURLE_OK) && retcode == 200) {
		printf("Request success.\n");
		printf("%s\n", content.c_str());
		return 0;
	}
	else {
		printf("Request RTMP URL from server failed.\n");
		return -3;
	}
}
/******************************************************************************************************************************
* Function    : finalizeCurl
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/04/06 10:46:20
* Modify Date : 2016/04/06 10:46:20
******************************************************************************************************************************/
void CLoginDialog::finalizeCurl(struct curl_slist *http_header, CURL *curl)
{
	curl_slist_free_all(http_header);
	curl_easy_cleanup(curl);
}
/******************************************************************************************************************************
* Function    : 
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/14 12:17:10
* Modify Date : 2016/03/14 12:17:10
******************************************************************************************************************************/
int CLoginDialog::requestLogin()
{
	CKaiboCaptureApp* theApp = (CKaiboCaptureApp*)AfxGetApp();

	unsigned char md[16] = { 0 };
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, OS::StringT2UTF8(m_password.GetBuffer(0)).c_str(), m_password.GetLength());
	MD5_Final(md, &ctx);

	printf("%s\n", hexstr(md, MD5_DIGEST_LENGTH));
	
	// 连接应用服务器
	char posturl[2048];
	char postfields[1024];
	sprintf(posturl, POSTURL_LOGIN, theApp->m_servAddress.c_str(), theApp->m_servPort);
	
#if 1
	sprintf(postfields, POSTFIELDS_LOGIN, \
		OS::StringT2A(m_username.GetBuffer(0)).c_str(), \
		OS::StringT2A(m_password.GetBuffer(0)).c_str());
#else
	// md5 encrypt
	sprintf(postfields, POSTFIELDS_LOGIN, \
		OS::StringT2A(m_username.GetBuffer(0)).c_str(), \
		hexstr(md, MD5_DIGEST_LENGTH));
#endif	
	m_username.ReleaseBuffer();
	m_password.ReleaseBuffer();

	printf("Request %s?%s\n", 
		posturl, 
		postfields);
	int ret = 0;
	CURL* curl = NULL;
	struct curl_slist* http_header = NULL;
	std::string content;
	initCurl(http_header, curl, content);
	setCurlPost(curl, posturl, postfields);

	ret = performCurl(curl, content);
	if (ret < 0) {
		AfxMessageBox(_T("无法连接应用服务器"));
		return -1;
	}

	finalizeCurl(http_header, curl);

	// 解析应用服务器返回的json数据
	std::string url = parseJsonResponse(content);
	int pos = url.find("rtmp://");
	if (!url.empty() && pos != -1) {
		theApp->m_rtmpUrl = url;
		//theApp->m_rtmpUrl = "rtmp://livexz.xiaozaiwh.com:1935/live/mystream";
		//theApp->m_rtmpUrl = "rtmp://192.168.177.112:1935/live/stream2";
		//theApp->m_rtmpUrl = "rtmp://localhost:1935/live/stream2";
	} else if (url.find("rtmp://") == -1) {
		return -2;
	} else {
		AfxMessageBox(_T("请求rtmp服务器地址失败"));
		return -1;
	}

	return 0;
}
/******************************************************************************************************************************
* Function    : utf82Unicode
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/17 17:47:50
* Modify Date : 2016/03/17 17:47:50
******************************************************************************************************************************/
TCHAR* CLoginDialog::utf82Unicode(const char* utf, size_t *unicode_number)
{
	if (!utf || !strlen(utf)) {
		*unicode_number = 0;
		return NULL;
	}
	int dwUnicodeLen = MultiByteToWideChar(CP_UTF8, 0, utf, -1, NULL, 0);
	size_t num = dwUnicodeLen*sizeof(wchar_t);
	wchar_t *pwText = (wchar_t*)malloc(num);
	memset(pwText, 0, num);
	MultiByteToWideChar(CP_UTF8, 0, utf, -1, pwText, dwUnicodeLen);
	*unicode_number = dwUnicodeLen - 1;
	return (TCHAR*)pwText;
}

/******************************************************************************************************************************
* Function    : utf82UnicodeString
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/18 16:20:35
* Modify Date : 2016/03/18 16:20:35
******************************************************************************************************************************/
std::string utf82UnicodeString(const char* utf, size_t *unicode_number)
{
	if (!utf || !strlen(utf)) {
		*unicode_number = 0;
		return "";
	}
	int dwUnicodeLen = MultiByteToWideChar(CP_UTF8, 0, utf, -1, NULL, 0);
	size_t num = dwUnicodeLen*sizeof(wchar_t);
	wchar_t *pwText = (wchar_t*)malloc(num);
	memset(pwText, 0, num);
	MultiByteToWideChar(CP_UTF8, 0, utf, -1, pwText, dwUnicodeLen);
	*unicode_number = dwUnicodeLen - 1;

	std::string return_value;
	int len = WideCharToMultiByte(CP_ACP, 0, pwText, *unicode_number, NULL, 0, NULL, NULL);
	char *buffer = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, pwText, *unicode_number, buffer, len, NULL, NULL);
	buffer[len] = '\0';

	return_value.append(buffer);
	delete[] buffer;
	free(pwText);

	return return_value;
}
/******************************************************************************************************************************
* Function    : unicode2Utf8
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/04/06 10:46:42
* Modify Date : 2016/04/06 10:46:42
******************************************************************************************************************************/
char* unicode2Utf8(const char* unicode)
{
	int len;
	len = WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)unicode, -1, NULL, 0, NULL, NULL);
	char *szUtf8 = (char*)malloc(len + 1);
	memset(szUtf8, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)unicode, -1, szUtf8, len, NULL, NULL);
	return szUtf8;
}
/******************************************************************************************************************************
* Function    : 
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/16 15:05:57
* Modify Date : 2016/03/16 15:05:57
******************************************************************************************************************************/
std::string CLoginDialog::parseJsonResponse(std::string content)
{
	Json::Reader reader;
	Json::Value root;
	if (reader.parse(content, root)) {
		std::string	code	= root["code"].asString();
		int icode = atoi(code.c_str());

		if (icode < 0) {
			printf("code is error.\n");
			return "";
		}

		std::string	msg		= root["message"].asString();
		size_t size = msg.size();
		std::string pMsg = utf82UnicodeString(msg.c_str(), &size);

		if (APPSERVER_RESPONSE_CODE_SUCCESS == icode) {
			std::string url = root["datas"]["videoPath"].asString();
			
			int pos = url.rfind('/');
			++ pos;
			m_videocastid = url.substr(pos, url.size() - pos);
			printf("success\n");
			printf("url: %s, id: %s\n", url.c_str(), m_videocastid.c_str());

			return url;
		} else {
			char buff[512] = { 0 };
			sprintf(buff, "%s", pMsg.c_str());
			std::cout << pMsg << std::endl;

			CString str(buff);
			AfxMessageBox(str);

			return buff;
		}
	}
	return "";
}
/******************************************************************************************************************************
* Function    : savePasswd
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/04/06 10:46:56
* Modify Date : 2016/04/06 10:46:56
******************************************************************************************************************************/
int CLoginDialog::savePasswd()
{
	int check = ((CButton*)GetDlgItem(IDC_SAVE_PASSWD))->GetCheck();

	CString strpasswd;
	GetDlgItemText(IDC_PASSWORD, strpasswd);

	const TCHAR configFile[] = CONFIG_FILE_NAME;

	TCHAR szFilePath[MAX_PATH + 1];
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0;

	std::wstring wfullpath;
	std::string fullpath;
	wfullpath.assign(szFilePath);
	wfullpath.append(OS::StringT2W(configFile));

	if (!PathFileExists(wfullpath.c_str())) {
		AfxMessageBox(_T("缺失配置文件"));
		return -1;
	}

	WStringToString(wfullpath, fullpath);
	if (1 == check) {
		Config configSettings(fullpath);
		char * enc_output = Base64Encode(OS::StringT2UTF8(strpasswd.GetBuffer(0)).c_str(),
			strpasswd.GetLength(), true);

		configSettings.Add("password", enc_output);
		configSettings.Add("passwdchecked", 1);
		std::ofstream outs(fullpath);
		outs << configSettings;

		strpasswd.ReleaseBuffer();
	}
	else {
		Config configSettings(fullpath);
		configSettings.Add("password", "");
		configSettings.Add("passwdchecked", 0);
		std::ofstream outs(fullpath);
		outs << configSettings;
	}
	return 0;
}
/******************************************************************************************************************************
* Function    : hexstr
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\KaiboCapture
* Brief       : 
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/04/06 10:48:40
* Modify Date : 2016/04/06 10:48:40
******************************************************************************************************************************/
char* CLoginDialog::hexstr(unsigned char *buf, int len)
{
	const char *set = "0123456789abcdef";
	static char str[65], *tmp;
	unsigned char *end;
	if (len > 32)
		len = 32;
	end = buf + len;
	tmp = &str[0];
	while (buf < end) {
		*tmp++ = set[(*buf) >> 4];
		*tmp++ = set[(*buf) & 0xF];
		buf++;
	}
	*tmp = '\0';
	return str;
}

void CLoginDialog::OnBnClickedOk()
{
	// 连接应用服务器，请求rtmp服务器推流URL地址
	GetDlgItemText(IDC_USERNAME, m_username);
	GetDlgItemText(IDC_PASSWORD, m_password);

	if (m_username.IsEmpty()) {
		m_loginok = 0;
		AfxMessageBox(_T("用户名为空"));
		return;
	}
	if (m_password.IsEmpty()) {
		m_loginok = 0;
		AfxMessageBox(_T("密码为空"));
		return;
	}
	if (requestLogin() < 0) {
		m_loginok = 0;
		//AfxMessageBox(_T("用户名或密码错误"));
		return;
	}

	m_loginok = 1;
	savePasswd();

	CDialogEx::OnOK();
}
BOOL CLoginDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_loginok = 0;
	SetDlgItemText(IDC_USERNAME, _T("test2016"));
	//SetDlgItemText(IDC_PASSWORD, _T("123456"));
	loadConfig();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
