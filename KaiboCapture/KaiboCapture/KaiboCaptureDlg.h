
// KaiboCaptureDlg.h : 標頭檔
//

#pragma once
#include "../XiaozaiFilters/XiaozaiFilters.h"
#include "../XiaozaiFilters/PreviewLocalFilters.h"
#include "../XiaozaiFilters/PreviewRtmpFilters.h"
#include "curl/curl.h"
#include <list>

// CKaiboCaptureDlg 對話方塊
class CKaiboCaptureDlg : public CDialogEx
{
// 建構
public:
	CKaiboCaptureDlg(CWnd* pParent = NULL);	// 標準建構函式
	virtual ~CKaiboCaptureDlg();
	void	setStartTime();
	void	setEndTime();
	void	setDeviceList();
	void	setVideoDeviceList(CWnd* pwnd);
	void	setAudioDeviceList(CWnd* pwnd);
	void	setDevicesFormatAvaliable();
	void	setRtmpUrl();
	long	getSystemTime(int idc);
	void setUsername(CString& username) {
		m_username = username;
	}
	void setPassword(CString& password) {
		m_password = password;
	}
	void setServAddress(std::string& addr) {
		m_servAddress = addr;
	}
	void setPort(int port) {
		m_servPort = port;
	}
	void setVideocastId(std::string& id) {
		m_videocastid = id;
	}

	HRESULT previewLocal(HWND hwnd);
	HRESULT previewRtmpStream(HWND hwnd);
	int m_connectRtmpStatus;

	void StopLocalPreview();
	void StopRtmpPreview();
	void StopPreview();

	HRESULT switchMedia(CString str);

// 對話方塊資料
	enum { IDD = IDD_KAIBOCAPTURE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支援


private:
	CString			m_username;				// 用户名
	CString			m_password;				// 密码
	std::string		m_servAddress;			// 应用服务器地址
	int				m_servPort;				// 应用服务器监听端口

	int				m_play_status;			// 直播状态
	int				m_record_status;		// 录像状态
	int				m_pause_status;			// 暂停继续
	std::string		m_rtmpUrl;				// 推流地址
	std::string		m_videocastid;			// 推流id
	unsigned long long			m_starttime;
	unsigned long long			m_endtime;
	long			m_elapsed;
	int				m_preview_status;		// 预览状态
	int				m_allow_comment;		// 是否允许评论
	int				m_video_encrypt;		// 视频加密与否
	std::string		m_video_key;			// 视频加密密钥
	int				m_video_permission;		// 视频是否允许合法直播
	int				m_result;
	std::list<_tstring> m_audiodecs;
	std::list<_tstring> m_videodecs;

	int requestStartVideo();
	int requestEndVideo();
	int requestVideoPermission();
	int parseJsonResponse(std::string content);

	int initCurl(struct curl_slist*& http_header, CURL*& curl, std::string& content);
	int setCurlPost(CURL* curl, const char* posturl, const char* postfields);
	int performCurl(CURL* curl, std::string& content);
	void finalizeCurl(struct curl_slist *http_header, CURL *curl);

	void makedir(CString path);

// 程式碼實作
protected:
	HICON m_hIcon;

	CXiaozaiFilters* m_AVController = NULL;

	CPreviewLocalFilters* m_previewLocalController = NULL;
	CPreviewRtmpFilters* m_previewRtmpController = NULL;

	// 產生的訊息對應函式
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedStartVideo();
protected:
	afx_msg LRESULT OnVideoUpdate(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnBnClickedStopVideo();
	afx_msg void OnStnClickedTestvideo();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedVideoSavePath();
	afx_msg void OnBnClickedPauseVideo();
	afx_msg void OnBnClickedPreview();
	afx_msg void OnBnClickedMediaSwitch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedRecordVideo();
	afx_msg void OnBnClickedConfig();
	afx_msg void OnBnClickedVideoEncrypt();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
