#include "stdafx.h"
#include "config.h"
#include "Win32_Utils.h"
#include "FilterGuids.h"
#include "PreviewLocalController.h"

#pragma comment (lib,"d3d9.lib")
#define LOAD_YUV420P 1

PreviewLocalController::PreviewLocalController(LPUNKNOWN pUnk, HRESULT *phr) :
CUnknown(NAME("PreviewLocalController"), pUnk)
{

}

PreviewLocalController::PreviewLocalController() :
CUnknown(NAME("PreviewLocalController"), this)
{
}

PreviewLocalController::~PreviewLocalController()
{
	CleanUp();
}

//
// NonDelegatingQueryInterface
//
// Override this to say what interfaces we support where
//
STDMETHODIMP PreviewLocalController::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
	if (riid == IID_ISuperSettings) {
		*ppv = static_cast<ISuperSettings *>(this);
		this->AddRef();
		return S_OK;
	}

	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT STDMETHODCALLTYPE PreviewLocalController::CleanUp()
{
	//OS::CCriticalSection::AutoLock mylock(m_cs);
	if (m_sURL)
		SysFreeString(m_sURL);
	m_sURL = NULL;

	if (m_sURLArgs)
		SysFreeString(m_sURLArgs);
	m_sURLArgs = NULL;

	if (m_sRecordPath)
		SysFreeString(m_sRecordPath);
	m_sRecordPath = NULL;

	if (m_sRecordArgs)
		SysFreeString(m_sRecordArgs);
	m_sRecordArgs = NULL;

	if (m_sSrcFileName)
		SysFreeString(m_sSrcFileName);
	m_sSrcFileName = NULL;

	if (m_sSrcFileArgs)
		SysFreeString(m_sSrcFileArgs);
	m_sSrcFileArgs = NULL;

	if (m_sAVDevName)
		SysFreeString(m_sAVDevName);
	m_sAVDevName = NULL;

	//Clear 
	if (m_pDirect3DSurfaceRender)
		m_pDirect3DSurfaceRender->Release();
	if (m_pDirect3DDevice)
		m_pDirect3DDevice->Release();
	if (m_pDirect3D9)
		m_pDirect3D9->Release();

	return S_OK;
}

HRESULT PreviewLocalController::OpenFileorDevice(const BSTR sName, const BSTR argstr, FFMpegInputWapper& theWapper)
{
	//initialize the input ffmpeg wapper, always should set options before opening the device
	bool done = theWapper.set_options(argstr, 0);
	done = theWapper.open_input_device(OS::StringT2UTF8(sName).c_str());
	m_sName = sName;
	//done = theWapper.open_input_device(sName);
	if (!done)
		return E_UNEXPECTED;
	m_AudioDecoder.set_options(argstr, 0);
	AVCodecContext * pCodecCxt = theWapper.get_audio_stream_codec();
	//pCodecCxt->request_channel_layout = 0; 
	pCodecCxt->request_sample_fmt = AV_SAMPLE_FMT_S16;
	m_AudioDecoder.open_codec(pCodecCxt);

	m_VideoDecoder.set_options(argstr, 0);
	m_VideoDecoder.open_codec(theWapper.get_video_stream_codec());

	if (done)
	{
		CopyCodecCxtProperties();
		return S_OK;
	}
	else
		return E_UNEXPECTED;
}

//ISuperSettings
///
/// Set video source device
/// sDevName: device name
/// argstr: a string of options
///
HRESULT STDMETHODCALLTYPE PreviewLocalController::SetSourceDevice(const BSTR sDevName, const BSTR argstr)
{
	HRESULT res = S_OK;
	m_sAVDevName = SysAllocString(sDevName);
	m_sDeviceArgs = SysAllocString(argstr);


	return OpenFileorDevice(sDevName, argstr, m_DshowDeviceWapper);
}



///
/// Set the source file
///
HRESULT STDMETHODCALLTYPE PreviewLocalController::SetSourceFileName(const BSTR sFileName)
{
	HRESULT res = S_OK;
	m_sSrcFileName = SysAllocString(sFileName);
	m_bFileSource = true;
	return OpenFileorDevice(sFileName, NULL, m_SrcFileWapper);
}

HRESULT STDMETHODCALLTYPE PreviewLocalController::SetRTMPServer(const BSTR sURL, const BSTR argstr)
{
	m_sURL = SysAllocString(sURL);
	m_sURLArgs = SysAllocString(argstr);

	bool done = m_rtmpURLWapper.set_options(m_sURLArgs, 0);

	done = m_rtmpURLWapper.open_output(OS::StringT2UTF8(m_sURL).c_str(), true);
	if (!done) {
		printf("%s: FFMpegOutputWapper, open_output failed.\n", __FUNCTION__);
		return -1;
	}

	if (!m_AudioEncoder.IsOpen())
	{
		m_AudioEncoder.set_options(m_sURLArgs, 0);
		m_AudioEncoder.open_codec(m_rtmpURLWapper.get_audio_stream_codec());
	}

	if (!m_VideoEncoder.IsOpen())
	{
		m_VideoEncoder.set_options(m_sURLArgs, 0);
		m_VideoEncoder.open_codec(m_rtmpURLWapper.get_video_stream_codec());
	}
	m_LogoFilter.init(m_rtmpURLWapper.get_video_stream_codec());

	SysFreeString(m_sURL);
	SysFreeString(m_sURLArgs);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE PreviewLocalController::SetRecordPath(const BSTR outputPath, const BSTR argstr)
{
	return S_OK;
}
int PreviewLocalController::ProcessVideo(AVPacket* &encodeV, AVFrame* &myframe, bool &bNeedFree, AVPacket * mupacket)
{
	//Decoding the video frame
	myframe = m_VideoDecoder.decode_with_scale(mupacket, 0, 0, AV_PIX_FMT_YUV420P);
	if (myframe == NULL)
		return -1;
	int ret = 0;

	AVFrame * theFrame = NULL;
	if (1)
	{
		//Add a logo 
		theFrame = m_LogoFilter.AddOneFrame(myframe);
		//Put the decoded video frame into a list
		m_RawSamplePool.InsertStreamSample(m_VideoStreamIndex, av_frame_clone(theFrame));
	}
	else
	{
		m_RawSamplePool.InsertStreamSample(m_VideoStreamIndex, av_frame_clone(myframe));
	}

	//Send msg to the Window
	if (m_hMsgWnd != NULL)
		::PostMessage(m_hMsgWnd, m_iUpdateMsg, 0, NULL);

	//Encoding a frame 
	if (m_VideoDecoder.IsH264() != 1)
	{
		bNeedFree = true;
		if (m_VideoEncoder.IsOpen())
			encodeV = m_VideoEncoder.encode(theFrame);
	}
	else
	{
		bNeedFree = true;
		encodeV = av_packet_clone(mupacket);
	}
	if (theFrame) {
		av_frame_unref(theFrame);
	}

	if (m_rtmpURLWapper.IsOpened() && encodeV) {
		ret = m_rtmpURLWapper.write_video_frame(encodeV);
	}
	if (encodeV) {
		av_packet_unref(encodeV);
	}

	return ret;
}

int PreviewLocalController::ProcessAudio(AVPacket* &encodeV, AVFrame* &myframe, bool &bNeedFree, AVPacket * mupacket)
{
	//Decoding the audio frame 
	myframe = m_AudioDecoder.decode_with_resample(mupacket, 0, AV_SAMPLE_FMT_S16, 0);
	if (myframe == NULL)
		return -1;
	int ret = 0;
	//Put the decoded audio frame into a list
	//== CODE ADD BY ANYZ =================================================================//
	// Date: 2016/04/12 15:55:33
	// 暂时不要向队列中塞音频，尚没有从队列中取音频包的需求
	//m_RawSamplePool.InsertStreamSample(m_AudioStreamIndex, av_frame_clone(myframe));
	//== CODE END =========================================================================//
	

	//Encoding a frame 

	if (m_AudioDecoder.IsAAC() != 1)
	{
		bNeedFree = false;
		bool isopen = m_AudioEncoder.IsOpen();
		if (isopen)
		{
			encodeV = m_AudioEncoder.encode(myframe);
			if (m_rtmpURLWapper.IsOpened() && encodeV)
				ret = m_rtmpURLWapper.write_audio_frame(encodeV);

			if (encodeV)
				av_packet_unref(encodeV);

			std::list<AVPacket*> pktlist;
			ret = m_AudioEncoder.flushencoder(pktlist);
			if (ret > 0)
			{
				for each(AVPacket* pkt in pktlist)
				{
					if (m_rtmpURLWapper.IsOpened() && pkt)
						ret = m_rtmpURLWapper.write_audio_frame(pkt);
					if (pkt)
						av_packet_unref(pkt);
				}
			}
		}
	}
	else {
		bNeedFree = true;
		encodeV = av_packet_clone(mupacket);
		if (m_rtmpURLWapper.IsOpened() && encodeV)
			ret = m_rtmpURLWapper.write_audio_frame(encodeV);
		if (encodeV)
			av_packet_unref(encodeV);
	}
	return ret;
}

int PreviewLocalController::Run()
{
	AVPacket * mupacket = NULL;
	int i = 0;
	FFMpegInputWapper* theInput = NULL;

	MSG theMsg;

	bool bNeedFree = false;
	AVPacket* encodeV = NULL;
	AVFrame *myframe = NULL;

	if (m_DshowDeviceWapper.IsOpened()) //if we have devices opened
		theInput = &m_DshowDeviceWapper;
	else
		theInput = &m_SrcFileWapper;

	do {
		bool isQuit = false;
		//Message processing
		if (::PeekMessage(&theMsg, NULL, 0, 0, PM_REMOVE))
		{
			switch (theMsg.message)
			{
			case PREVIEW_LOCAL_QUIT:
				isQuit = true;
				break;
			default:
				break;
			}
		}
		if (isQuit) {
			//if (myframe)
			//	av_frame_unref(myframe);
			break;
		}
		//Read out a source frame
		mupacket = theInput->get_a_frame();
		if (mupacket == NULL)
			break;

		if (mupacket->stream_index == theInput->get_videostream_index())
		{
			ProcessVideo(encodeV, myframe, bNeedFree, mupacket);
		}
		else if (mupacket->stream_index == theInput->get_audiostream_index())
		{
			ProcessAudio(encodeV, myframe, bNeedFree, mupacket);
		}

		//Release the internal buffer of the frame
		//if (myframe)
		//	av_frame_unref(myframe);

		//Release encoded buffer
		if (encodeV)
		{
			if (bNeedFree)
			{
				av_packet_unref(encodeV);
				//av_packet_free(&encodeV);
			}
		}

		//Release packet
		if (mupacket) {
			av_packet_unref(mupacket);
			//av_packet_free(&mupacket);
		}
		//Write 

	} while (true);

	//flush the frames in video decoder
	AVPacket packet;
	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;
	while (true)
	{
		//TODO: fan-out the last frames
		int ret = ProcessVideo(encodeV, myframe, bNeedFree, &packet);
		if (ret < 0)
			break;
		//Release encoded buffer
		if (encodeV)
		{
			if (bNeedFree)
			{
				av_packet_unref(encodeV);
				//av_packet_free(&encodeV);
			}
		}
		//Release the internal buffer of the frame
		if (myframe)
			av_frame_unref(myframe);
		if (packet.buf)
			av_packet_unref(&packet);

		ret = ProcessAudio(encodeV, myframe, bNeedFree, &packet);
		if (ret < 0)
			break;

		//Release encoded buffer
		if (encodeV)
		{
			if (bNeedFree)
			{
				av_packet_unref(encodeV);
				//av_packet_free(&encodeV);
			}
		}
		//Release the internal buffer of the frame
		if (myframe)
			av_frame_unref(myframe);
		if (packet.buf)
			av_packet_unref(&packet);
	}

	//Flush the video frames in video encoder
	std::list<AVPacket*> pktlist;
	int ret = m_VideoEncoder.flushencoder(pktlist);
	if (ret > 0)
	{
		for each(AVPacket* pkt in pktlist)
		{
			if (m_rtmpURLWapper.IsOpened() && pkt)
				ret = m_rtmpURLWapper.write_video_frame(pkt);

			av_packet_free(&pkt);
		}
	}
	m_rtmpURLWapper.flush();

	return 0;
}
/*
// 内存泄漏版本
int PreviewLocalController::ProcessVideo(AVPacket* &encodeV, AVFrame* &myframe, bool &bNeedFree, AVPacket * mupacket)
{
	
	//Decoding the video frame
	myframe = m_VideoDecoder.decode_with_scale(mupacket, 0, 0, AV_PIX_FMT_YUV420P);
	if (myframe == NULL)
		return -1;
	int ret = 0;

	AVFrame* theFrame = NULL;
	if (1) {
		//Add a logo 
		theFrame = m_LogoFilter.AddOneFrame(myframe);
		//Put the decoded video frame into a list
		m_RawSamplePool.InsertStreamSample(m_VideoStreamIndex, av_frame_clone(theFrame));
	} else {
		m_RawSamplePool.InsertStreamSample(m_VideoStreamIndex, av_frame_clone(myframe));
	}

	//Send msg to the Window
	if (m_hMsgWnd != NULL)
		::PostMessage(m_hMsgWnd, m_iUpdateMsg, 0, NULL);

	//Encoding a frame 
	if (m_VideoDecoder.IsH264() != 1) {
		bNeedFree = true;
		if (m_VideoEncoder.IsOpen())
			encodeV = m_VideoEncoder.encode(theFrame);
	} else {
		bNeedFree = true;
		encodeV = av_packet_clone(mupacket);
	}
	if (theFrame) {
		av_frame_unref(theFrame);
	}

	//Write to a file
	if (m_rtmpURLWapper.IsOpened() && encodeV){
		ret = m_rtmpURLWapper.write_video_frame(encodeV);
	}
	if (encodeV) {
		av_packet_unref(encodeV);
	}
	return ret;
}

int PreviewLocalController::ProcessAudio(AVPacket* &encodeV, AVFrame* &myframe, bool &bNeedFree, AVPacket * mupacket)
{
	//Decoding the audio frame 
	myframe = m_AudioDecoder.decode_with_resample(mupacket, 0, AV_SAMPLE_FMT_S16, 0);
	if (myframe == NULL)
		return -1;
	int ret = 0;
	//Put the decoded audio frame into a list
	m_RawSamplePool.InsertStreamSample(m_AudioStreamIndex, av_frame_clone(myframe));

	//Encoding a frame 

	if (m_AudioDecoder.IsAAC() != 1)
	{
		bNeedFree = false;
		bool isopen = m_AudioEncoder.IsOpen();
		if (isopen)
		{
			encodeV = m_AudioEncoder.encode(myframe);
			if (m_rtmpURLWapper.IsOpened() && encodeV)
				ret = m_rtmpURLWapper.write_audio_frame(encodeV);

			std::list<AVPacket*> pktlist;
			ret = m_AudioEncoder.flushencoder(pktlist);
			if (ret > 0)
			{
				for each(AVPacket* pkt in pktlist)
				{
					if (m_rtmpURLWapper.IsOpened() && pkt)
						ret = m_rtmpURLWapper.write_audio_frame(pkt);

					av_packet_unref(pkt);
				}
			}
		}
	}
	else
	{
		bNeedFree = true;
		encodeV = av_packet_clone(mupacket);

		if (m_rtmpURLWapper.IsOpened() && encodeV)
			ret = m_rtmpURLWapper.write_audio_frame(encodeV);
	}

	if (encodeV)
		av_packet_unref(encodeV);
	return ret;
}
int PreviewLocalController::Run()
{
	AVPacket * mupacket = NULL;
	int i = 0;
	FFMpegInputWapper* theInput = NULL;

	MSG theMsg;

	bool bNeedFree = false;
	AVPacket* encodeV = NULL;
	AVFrame *myframe = NULL;

	if (m_DshowDeviceWapper.IsOpened()) //if we have devices opened
		theInput = &m_DshowDeviceWapper;
	else
		theInput = &m_SrcFileWapper;

	do {
		bool isQuit = false;
		//Message processing
		if (::PeekMessage(&theMsg, NULL, 0, 0, PM_REMOVE))
		{
			switch (theMsg.message)
			{
			case PREVIEW_LOCAL_QUIT:
				isQuit = true;
				break;
			default:
				break;
			}
		}
		if (isQuit) {
			if (myframe)
				av_frame_unref(myframe);
			break;
		}
		//Read out a source frame
		mupacket = theInput->get_a_frame();
		if (mupacket == NULL)
			break;

		if (mupacket->stream_index == theInput->get_videostream_index())
		{
			ProcessVideo(encodeV, myframe, bNeedFree, mupacket);
		}
		else if (mupacket->stream_index == theInput->get_audiostream_index())
		{
			ProcessAudio(encodeV, myframe, bNeedFree, mupacket);
		}
		//Release encoded buffer
		if (encodeV)
		{
			if (bNeedFree)
			{
				av_packet_unref(encodeV);
				//av_packet_free(&encodeV);
			}
		}
		//Release packet
		if (mupacket) {
			av_packet_unref(mupacket);
			//av_packet_free(&mupacket);
		}
		//Write 

	} while (true);

	//flush the frames in video decoder
	AVPacket packet;
	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;
	while (true)
	{
		//TODO: fan-out the last frames
		int ret = ProcessVideo(encodeV, myframe, bNeedFree, &packet);
		if (ret < 0)
			break;
		//Release encoded buffer
		if (encodeV)
		{
			if (bNeedFree)
			{
				av_packet_unref(encodeV);
				av_packet_free(&encodeV);
			}
		}
		//Release the internal buffer of the frame
		if (myframe)
			av_frame_unref(myframe);
		if (packet.buf)
			av_packet_unref(&packet);

		ret = ProcessAudio(encodeV, myframe, bNeedFree, &packet);
		if (ret < 0)
			break;

		//Release encoded buffer
		if (encodeV)
		{
			if (bNeedFree)
			{
				av_packet_unref(encodeV);
			}
		}
		//Release the internal buffer of the frame
		if (myframe)
			av_frame_unref(myframe);
		if (packet.buf)
			av_packet_unref(&packet);		
	}

	m_rtmpURLWapper.flush();

	return 0;
}
*/
HRESULT STDMETHODCALLTYPE PreviewLocalController::FillMediaSample(BYTE* pData, LONG lDataLen, BOOL bVideo, REFERENCE_TIME *rtStart, REFERENCE_TIME *rtEnd, LONG *size)
{
	AVFrame* theFrame = NULL;
	HRESULT hres = S_OK;
	BYTE * pBuf = pData;
	if (bVideo)
	{
		hres = m_RawSamplePool.GetStreamSample(m_VideoStreamIndex, &theFrame);
		if (hres != S_OK)
			return hres;
		*size = 0;
		for (int linesize = 0; linesize < AV_NUM_DATA_POINTERS; linesize++)
		{
			*size += abs(theFrame->linesize[linesize]*theFrame->height);
		}

		if (lDataLen < *size)
		{
			return E_FAIL;
		}
		//Fill data
		for (int linesize = 0; linesize < AV_NUM_DATA_POINTERS; linesize++)
		{
			int len = theFrame->linesize[linesize] * theFrame->height;
			if (len < 0)
			{
				BYTE* pSrc = theFrame->data[linesize] + theFrame->linesize[linesize] * (theFrame->height - 1);
				len = -len;
				CopyMemory(pBuf, pSrc, len);				
			}
			else
			{
				CopyMemory(pBuf, theFrame->data[linesize], len);
			}
			pBuf += len;  //move the pointer
		}
		*rtStart = theFrame->pkt_pts;
		*rtEnd = theFrame->pkt_pts + theFrame->pkt_duration;
	}
	else
	{
		hres = m_RawSamplePool.GetStreamSample(m_AudioStreamIndex, &theFrame);
		if (hres != S_OK)
			return hres;
		//Note: Here, we assume the format is LRLR, and is not a plannar format
		int len = theFrame->linesize[0];
		CopyMemory(pBuf, theFrame->data[0], len);
		*rtStart = theFrame->pkt_pts;
		*rtEnd = theFrame->pkt_pts + theFrame->pkt_duration;
	}

	av_frame_unref(theFrame);
	av_frame_free(&theFrame);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE PreviewLocalController::GetAudioInfo(PWORD nChannels,         // Number of channels
	PDWORD nSamplesPerSec,   // Samples per second
	PWORD wBitsPerSample     // Bits per sample
	)
{
	if (m_DshowDeviceWapper.IsOpened())
	{
		if (m_DshowDeviceWapper.get_audio_info(*nChannels, *nSamplesPerSec, *wBitsPerSample) < 0)
			return E_NOT_VALID_STATE;
		return S_OK;
	}
	else if (m_SrcFileWapper.IsOpened())
	{
		if (m_SrcFileWapper.get_audio_info(*nChannels, *nSamplesPerSec, *wBitsPerSample) < 0)
			return E_NOT_VALID_STATE;
		return S_OK;
	}
	else
	{
		return E_NOT_VALID_STATE;
	}
}

HRESULT STDMETHODCALLTYPE PreviewLocalController::GetVideoInfo(PLONG Width, PLONG Height, DOUBLE* fps)
{
	if (m_DshowDeviceWapper.IsOpened())
	{
		if (m_DshowDeviceWapper.get_video_info(*Width, *Height, *fps) < 0)
			return E_NOT_VALID_STATE;
	}
	else if (m_SrcFileWapper.IsOpened())
	{
		if (m_SrcFileWapper.get_video_info(*Width, *Height, *fps) < 0)
			return E_NOT_VALID_STATE;
	}
	else
	{
		return E_NOT_VALID_STATE;
	}

	return S_OK;
}

//Run with stand alone mode
HRESULT STDMETHODCALLTYPE PreviewLocalController::StarPlay(BOOL bWithAudio)
{
	Start();	//Start the thread
	return S_OK;
}


HRESULT STDMETHODCALLTYPE PreviewLocalController::StopPlay()
{
	OS::CCriticalSection::AutoLock mylock(m_cs);
	this->PostMessageW(PREVIEW_LOCAL_QUIT, 0, NULL);
	this->Wait(1000);
	this->Terminate();
	m_RawSamplePool.ClearAllSamples();
	m_DshowDeviceWapper.close_input_device();

	if (m_VideoEncoder.IsOpen())
		m_VideoEncoder.close_codec();
	if (m_AudioEncoder.IsOpen())
		m_AudioEncoder.close_codec();

	if (m_rtmpURLWapper.IsOpened())
		m_rtmpURLWapper.CleanUp();

	ReleaseD3D();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE PreviewLocalController::PausePlay()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PreviewLocalController::ResumePlay()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PreviewLocalController::EnableAudio()
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE PreviewLocalController::DisableAudio()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PreviewLocalController::SetMessageLoopMode(HWND hMsgWnd, HWND hDspWnd, UINT iMsg)
{
	//OS::CCriticalSection::AutoLock mylock(m_cs);
	if (m_running)
		return E_FAIL;
	m_bMessageLoopMode = TRUE;
	m_hMsgWnd = hMsgWnd;
	m_hDspWnd = hDspWnd;
	m_iUpdateMsg = iMsg;
	//init Direct3D
	LONG  lVideoWidth;
	LONG  lVideoHeight;
	DOUBLE fps;
	GetVideoInfo(&lVideoWidth, &lVideoHeight, &fps);
	InitD3D(m_hDspWnd,lVideoWidth,lVideoHeight);
	return S_OK;
}

int PreviewLocalController::InitD3D(HWND hwnd, unsigned long lWidth, unsigned long lHeight)
{
    HRESULT lRet;
    m_pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (m_pDirect3D9 == NULL)
        return -1;

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

    GetClientRect(hwnd, &m_rtViewport);

    lRet = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp, &m_pDirect3DDevice);
    if (FAILED(lRet))
        return -1;

#ifdef LOAD_BGRA
    lRet = m_pDirect3DDevice->CreateOffscreenPlainSurface(
        lWidth, lHeight,
        D3DFMT_X8R8G8B8,
        D3DPOOL_DEFAULT,
        &m_pDirect3DSurfaceRender,
        NULL);
#elif LOAD_YUV420P
    lRet = m_pDirect3DDevice->CreateOffscreenPlainSurface(
        lWidth, lHeight,
		        (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'),
        D3DPOOL_DEFAULT,
        &m_pDirect3DSurfaceRender,
        NULL);
#endif


    if (FAILED(lRet))
        return -1;

    return 0;
}


HRESULT STDMETHODCALLTYPE PreviewLocalController::RenderVideoFrame()
{
    HRESULT lRet;

    if (m_pDirect3DSurfaceRender == NULL)
        return -1;
	
	if (!m_running)
		return -1;

	//Obtain a frame
	AVFrame * theFrame;
	lRet = m_RawSamplePool.GetStreamSample(m_VideoStreamIndex, &theFrame);
	
	if (lRet != S_OK)
		return lRet;
	if (!theFrame)
		return -1;
	
    D3DLOCKED_RECT d3d_rect;
	lRet = m_pDirect3DSurfaceRender->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
    if (FAILED(lRet))
		return -1;


    BYTE * pDest = (BYTE *)d3d_rect.pBits;
    int stride = d3d_rect.Pitch;
    int i = 0;
	BYTE * pSrc = theFrame->data[0];
	   
	//Copy Data  
#ifdef LOAD_BGRA
    int pixel_w_size = pixel_w * 4;
    for (i = 0; i< pixel_h; i++){
        memcpy(pDest, pSrc, pixel_w_size);
        pDest += stride;
        pSrc += pixel_w_size;

	}
#elif LOAD_YUV420P
    for (i = 0; i < theFrame->height; i++){
		memcpy(pDest, pSrc, theFrame->width);
		pDest += stride;
		pSrc += theFrame->width;
	}
	pSrc = theFrame->data[2]; //Component V first
	for (i = 0; i < theFrame->height / 2; i++){
		memcpy(pDest, pSrc, theFrame->width / 2);
		pDest += stride/2;
		pSrc += theFrame->width / 2;
	}
	pSrc = theFrame->data[1]; //Component U
	for (i = 0; i < theFrame->height / 2; i++){
		memcpy(pDest, pSrc, theFrame->width / 2);
		pDest += stride / 2;
		pSrc += theFrame->width / 2;
	}
#endif
	av_frame_unref(theFrame);
	av_frame_free(&theFrame);

    lRet = m_pDirect3DSurfaceRender->UnlockRect();

    if (FAILED(lRet))
        return -1;

    if (m_pDirect3DDevice == NULL)
		return -1;

	m_pDirect3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	m_pDirect3DDevice->BeginScene();
	IDirect3DSurface9 * pBackBuffer = NULL;

    m_pDirect3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);


    m_pDirect3DDevice->StretchRect(m_pDirect3DSurfaceRender, NULL, pBackBuffer, &m_rtViewport, D3DTEXF_LINEAR);
    m_pDirect3DDevice->EndScene();
    m_pDirect3DDevice->Present(NULL, NULL, NULL, NULL);

	Safe_Release(pBackBuffer);

    return true;
}

int PreviewLocalController::CopyCodecCxtProperties()
{
	FFMpegInputWapper * srcWapper = NULL;
	if (m_DshowDeviceWapper.IsOpened())
		srcWapper = &m_DshowDeviceWapper;
	else
		srcWapper = &m_SrcFileWapper;

	FFMpegAudioInfo audioinfo;
	FFMpegVideoInfo videoinfo;
	srcWapper->get_audio_info(audioinfo);
	//audioinfo.bit_rate = 64000; //should be set by option string
	//audioinfo.channel_layout = AV_CH_LAYOUT_STEREO; //should be set by option string

	srcWapper->get_video_info(videoinfo);
	//videoinfo.bit_rate = 800000; //should be set by option string
	/*if (videoinfo.framerate.num && videoinfo.framerate.den)
	{
		videoinfo.gop_size = round(((float)videoinfo.framerate.num) / ((float)videoinfo.framerate.den));
	}
	else if (videoinfo.time_base.num)
	{

		videoinfo.gop_size = round(((float)videoinfo.time_base.den) / ((float)videoinfo.time_base.num));
	}
	else
	{
		videoinfo.gop_size = 25;
	}*/
	videoinfo.thread_count = 8;
	videoinfo.pix_fmt = AV_PIX_FMT_YUV420P;
	videoinfo.ticks_per_frame = 2;
	

	m_rtmpURLWapper.set_audio_info(audioinfo);
	m_rtmpURLWapper.set_video_info(videoinfo);

	m_recordFileWapper.set_audio_info(audioinfo);
	m_recordFileWapper.set_video_info(videoinfo);

	return 0;
}
HRESULT PreviewLocalController::ShowCameraProps()
{
	return E_NOTIMPL;
}