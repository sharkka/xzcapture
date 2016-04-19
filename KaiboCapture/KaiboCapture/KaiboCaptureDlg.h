
// KaiboCaptureDlg.h : ���^�n
//

#pragma once
#include "../XiaozaiFilters/XiaozaiFilters.h"
#include "../XiaozaiFilters/PreviewLocalFilters.h"
#include "../XiaozaiFilters/PreviewRtmpFilters.h"
#include "curl/curl.h"
#include <list>

// CKaiboCaptureDlg ��Ԓ���K
class CKaiboCaptureDlg : public CDialogEx
{
// ����
public:
	CKaiboCaptureDlg(CWnd* pParent = NULL);	// �˜ʽ�����ʽ
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

// ��Ԓ���K�Y��
	enum { IDD = IDD_KAIBOCAPTURE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧Ԯ


private:
	CString			m_username;				// �û���
	CString			m_password;				// ����
	std::string		m_servAddress;			// Ӧ�÷�������ַ
	int				m_servPort;				// Ӧ�÷����������˿�

	int				m_play_status;			// ֱ��״̬
	int				m_record_status;		// ¼��״̬
	int				m_pause_status;			// ��ͣ����
	std::string		m_rtmpUrl;				// ������ַ
	std::string		m_videocastid;			// ����id
	unsigned long long			m_starttime;
	unsigned long long			m_endtime;
	long			m_elapsed;
	int				m_preview_status;		// Ԥ��״̬
	int				m_allow_comment;		// �Ƿ���������
	int				m_video_encrypt;		// ��Ƶ�������
	std::string		m_video_key;			// ��Ƶ������Կ
	int				m_video_permission;		// ��Ƶ�Ƿ�����Ϸ�ֱ��
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

// ��ʽ�a����
protected:
	HICON m_hIcon;

	CXiaozaiFilters* m_AVController = NULL;

	CPreviewLocalFilters* m_previewLocalController = NULL;
	CPreviewRtmpFilters* m_previewRtmpController = NULL;

	// �a����ӍϢ������ʽ
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
