#pragma once


MIDL_INTERFACE("3F317BE7-2CAF-4A8F-8D7D-3406609B2ABA")
ISuperSettings : public IUnknown
{
public:
	///
	/// Set video source device
	/// sDevName: device name
	/// argstrV: a string of options, should be lowercase. the options should be in a format as follow:
	/// seprated by "$", 
	/// "-codev:" for video codec, "-codea:" for audio codec, "-fmt:" for file format options
	/// "-fmt:flush_packets=1$-codecv:width=480;height=320;r=25$-codeca:width=480;height=320;r=25"
	///
	virtual HRESULT STDMETHODCALLTYPE SetSourceDevice(const BSTR sDevName, const BSTR argstr) = 0;


	///
	/// Set the source file
	///
	virtual HRESULT STDMETHODCALLTYPE SetSourceFileName(const BSTR sFileName) = 0;

	///
	/// Set the RTMP server we would like to push our stream to
	/// sURL: URL of the server
	/// argstr: a string of options
	///
	virtual HRESULT STDMETHODCALLTYPE SetRTMPServer(const BSTR sURL, const BSTR argstr) = 0;

	///
	/// Set the local file dump destination
	/// outputPath: path of the local file
	/// argstr: a string of options
	///
	virtual HRESULT STDMETHODCALLTYPE SetRecordPath(const BSTR outputPath, const BSTR argstr) = 0;

	virtual HRESULT STDMETHODCALLTYPE CleanUp()=0;

	///
	/// Set the system runs as a standalone program with Message Processing Loop. 
	/// Once a new Video frame needs display, the system will send a Windows Message to the Window of hMsgWnd.
	/// The message's id is iMsg.
	/// hDspWnd is used to display the video frame. 
	/// We design such an interface because windows UI cannot support multi-thread, all GUI related updating will be in a UIThread.
	///
	virtual HRESULT STDMETHODCALLTYPE SetMessageLoopMode(HWND hMsgWnd, HWND hDspWnd, UINT iMsg) = 0;

	///
	/// This should only be called when the iMsg message is received by the UIThread.
	///
	virtual HRESULT STDMETHODCALLTYPE RenderVideoFrame() = 0;
	//
	// Set the system runs as a DShow filter
	//
	virtual HRESULT STDMETHODCALLTYPE SetDShowMode() = 0;
	//Run with stand alone mode
	virtual HRESULT STDMETHODCALLTYPE StarPlay(BOOL bWithAudio) = 0;
	virtual HRESULT STDMETHODCALLTYPE PausePlay() = 0;
	virtual HRESULT STDMETHODCALLTYPE ResumePlay() = 0;
	virtual HRESULT STDMETHODCALLTYPE StopPlay() = 0;
	virtual HRESULT STDMETHODCALLTYPE ShowCameraProps() = 0;

	virtual HRESULT STDMETHODCALLTYPE FillMediaSample(BYTE* pData, LONG lDataLen, BOOL bVideo, REFERENCE_TIME *rtStart, REFERENCE_TIME *rtEnd, LONG *size) = 0;

	///all parameters are for output
	virtual HRESULT STDMETHODCALLTYPE GetAudioInfo(PWORD nChannels,         // Number of channels
		PDWORD nSamplesPerSec,   // Samples per second
		PWORD wBitsPerSample     // Bits per sample
		) = 0;

	///all parameters are for output
	virtual HRESULT STDMETHODCALLTYPE GetVideoInfo(PLONG Width, PLONG Height, DOUBLE* fps) = 0;

	virtual HRESULT STDMETHODCALLTYPE EnableAudio() = 0;
	virtual HRESULT STDMETHODCALLTYPE DisableAudio() = 0;

};// END INTERFACE DEFINITION IRecordProvider