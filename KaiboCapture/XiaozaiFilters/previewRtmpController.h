#pragma once
#include "Config.h"
#include "ISuperSettings.h"
#include "Thread.h"
#include "FFMpegOutputWapper.h"
#include "FFMpegInputWapper.h"
#include "FFMpegEncoder.h"
#include "FFMpegDecoder.h"
#include "SampleQueue.h"
#include "FFMpegFilter.h"
#include <d3d9.h>  

#define Safe_Release(p) if((p)) {(p)->Release(); }

#define PREVIEW_RTMP_QUIT    WM_USER+21
#define PREVIEW_RTMP_UPDATE  WM_USER+22

class  PreviewRtmpController :public CUnknown, public ISuperSettings, public OS::CThread
{
	OS::CCriticalSection m_cs;
	BOOL  m_bAudioEnabled = TRUE;
	BOOL  m_bMessageLoopMode = TRUE;
	HWND  m_hMsgWnd = NULL; //the handle of Window for message processing
	HWND  m_hDspWnd = NULL; //the handle of Window for video display
	UINT  m_iUpdateMsg = PREVIEW_RTMP_UPDATE;
	RECT m_rtViewport;

	WORD  m_VideoStreamIndex = 0;
	WORD  m_AudioStreamIndex = 1;
	
	//Record related members
	BSTR	m_sRecordPath;				//record path
	BSTR    m_sRecordArgs;				//the arguments of record file
	FFMpegOutputWapper m_recordFileWapper;    //the record file

	//RTMP streaming push URL
	BSTR	m_sURL;
	BSTR	m_sURLArgs;
	FFMpegOutputWapper m_rtmpURLWapper;		//the push RTMP server URL

	//Source devices
	BSTR m_sAVDevName = NULL;
	BSTR m_sDeviceArgs;
	FFMpegInputWapper m_DshowDeviceWapper;	//Source video device

	//Source file
	BSTR m_sSrcFileName;
	BSTR m_sSrcFileArgs;
	FFMpegInputWapper m_SrcFileWapper;	//Source audio device
	bool m_bFileSource = false; //default, we use Device as our input
	FFMpegVideoDecoder m_VideoDecoder; //only when streaming from a file
	FFMpegAudioDecoder m_AudioDecoder; //only when streaming from a file

	//Video encoding
	
	FFMpegAudioEncoder m_AudioEncoder;
	FFMpegVideoEncoder m_VideoEncoder;

	FFMpegLogoFilter m_LogoFilter;

	CStreamSamplePool m_RawSamplePool;
	CStreamSamplePool m_CompressedPool;

	IDirect3D9 *m_pDirect3D9 = NULL;
	IDirect3DDevice9 *m_pDirect3DDevice = NULL;
	IDirect3DSurface9 *m_pDirect3DSurfaceRender = NULL;


	int ProcessVideo(AVPacket* &encodeV, AVFrame* &myframe, bool &bNeedFree, AVPacket * mupacket);
	int ProcessAudio(AVPacket* &encodeV, AVFrame* &myframe, bool &bNeedFree, AVPacket * mupacket);
	HRESULT OpenFileorDevice(const BSTR sDevName, const BSTR argstr, FFMpegInputWapper& theWapper);

public:
	PreviewRtmpController(LPUNKNOWN pUnk, HRESULT *phr);

	PreviewRtmpController();

	virtual ~PreviewRtmpController();

	DECLARE_IUNKNOWN

	// Overridden to say what interfaces we support where
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	//ISuperSettings
	///
	/// Set video source device
	/// sDevName: device name
	/// argstr: a string of options
	///
	HRESULT STDMETHODCALLTYPE SetSourceDevice(const BSTR sDevName, const BSTR argstr);


	///
	/// Set the source file
	///
	HRESULT STDMETHODCALLTYPE SetSourceFileName(const BSTR sFileName);

	///
	/// Set the RTMP server we would like to push our stream to
	/// sURL: URL of the server
	/// argstr: a string of options
	///
	HRESULT STDMETHODCALLTYPE SetRTMPServer(const BSTR sURL, const BSTR argstr);

	///
	/// Set the local file dump destination
	/// outputPath: path of the local file
	/// argstr: a string of options
	///
	HRESULT STDMETHODCALLTYPE SetRecordPath(const BSTR outputPath, const BSTR argstr);

	HRESULT STDMETHODCALLTYPE CleanUp();

	virtual HRESULT STDMETHODCALLTYPE SetMessageLoopMode(HWND hMsgWnd, HWND hDspWnd, UINT iMsg);
	virtual HRESULT STDMETHODCALLTYPE RenderVideoFrame();

	//
	// Set the system runs as a DShow filter
	//
	virtual HRESULT STDMETHODCALLTYPE SetDShowMode(){ m_bMessageLoopMode = false; return S_OK; }

	//Run with stand alone mode
	virtual HRESULT STDMETHODCALLTYPE StarPlay(BOOL bWithAudio);
	virtual HRESULT STDMETHODCALLTYPE StopPlay();
	virtual HRESULT STDMETHODCALLTYPE PausePlay();
	virtual HRESULT STDMETHODCALLTYPE ResumePlay();

	///all parameters are for output
	virtual HRESULT STDMETHODCALLTYPE GetAudioInfo(PWORD nChannels,         // Number of channels
		PDWORD nSamplesPerSec,   // Samples per second
		PWORD wBitsPerSample     // Bits per sample
		);

	///all parameters are for output
	virtual HRESULT STDMETHODCALLTYPE GetVideoInfo(PLONG Width, PLONG Height, DOUBLE* fps);

	virtual HRESULT STDMETHODCALLTYPE EnableAudio();
	virtual HRESULT STDMETHODCALLTYPE DisableAudio();

	virtual HRESULT STDMETHODCALLTYPE FillMediaSample(BYTE* pData, LONG lDataLen, BOOL bVideo, REFERENCE_TIME *rtStart, REFERENCE_TIME *rtEnd, LONG *size);
	
	//Filter thread
	virtual void Start()
	{

		m_RawSamplePool.AddQueue(m_VideoStreamIndex);		
		m_RawSamplePool.AddQueue(m_AudioStreamIndex);

		CThread::Start();
	};
	virtual HRESULT STDMETHODCALLTYPE ShowCameraProps();
protected:
	int Run();
	int InitD3D(HWND hwnd, unsigned long lWidth, unsigned long lHeight);
	void ReleaseD3D() {
		OS::CCriticalSection::AutoLock mylock(m_cs);
		Safe_Release(m_pDirect3D9);
		Safe_Release(m_pDirect3DDevice);
		Safe_Release(m_pDirect3DSurfaceRender);
	}
	int CopyCodecCxtProperties();

private:
	BSTR m_sName;
};

