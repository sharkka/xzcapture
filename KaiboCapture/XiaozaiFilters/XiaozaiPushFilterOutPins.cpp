#include "stdafx.h"
//#include "dshowutil.h"
#include <streams.h>
//#include "wxutil.h"
//#include "amfilter.h"

#include "Uploader.h"
#include "XiaozaiPushFilterOutPins.h"

const LONGLONG ONE_SECOND = 10000000;  // One second, in 100-nanosecond units.
const LONGLONG ONE_MSEC = ONE_SECOND / 10000;  // One millisecond, in 100-nanosecond units

///////////////////////////////////////////////////////////////////////
// Name: CreatePCMAudioType
// Desc: Initialize a PCM audio type with a WAVEFORMATEX format block.
//       (This function does not handle WAVEFORMATEXTENSIBLE formats.)
//
// If the method succeeds, call FreeMediaType to free the format block.
///////////////////////////////////////////////////////////////////////

inline HRESULT CreatePCMAudioType(
	AM_MEDIA_TYPE& mt,      // Media type to populate
	WORD nChannels,         // Number of channels
	DWORD nSamplesPerSec,   // Samples per second
	WORD wBitsPerSample     // Bits per sample
	)
{
	FreeMediaType(mt);


	mt.pbFormat = (BYTE*)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
	if (!mt.pbFormat)
	{
		return E_OUTOFMEMORY;
	}
	mt.cbFormat = sizeof(WAVEFORMATEX);

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_PCM;
	mt.formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEX *pWav = (WAVEFORMATEX*)mt.pbFormat;
	pWav->wFormatTag = WAVE_FORMAT_PCM;
	pWav->nChannels = nChannels;
	pWav->nSamplesPerSec = nSamplesPerSec;
	pWav->wBitsPerSample = wBitsPerSample;
	pWav->cbSize = 0;

	// Derived values
	pWav->nBlockAlign = nChannels * (wBitsPerSample / 8);
	pWav->nAvgBytesPerSec = nSamplesPerSec * pWav->nBlockAlign;

	return S_OK;
}

///////////////////////////////////////////////////////////////////////
// FramesPerSecToFrameLength
// Converts from frames-to-second to frame duration.
///////////////////////////////////////////////////////////////////////

inline REFERENCE_TIME FramesPerSecToFrameLength(double fps)
{
	return (REFERENCE_TIME)((double)ONE_SECOND / fps);
}

///////////////////////////////////////////////////////////////////////
// Name: CreateRGBVideoType
// Desc: Initialize an uncompressed RGB video media type.
//       (Allocates the palette table for palettized video)
//
// mt:         Media type to populate
// iBitDepth:  Bits per pixel. Must be 1, 4, 8, 16, 24, or 32
// Width:      Width in pixels
// Height:     Height in pixels. Use > 0 for bottom-up DIBs, < 0 for top-down DIB
// fps:        Frame rate, in frames per second
//
// If the method succeeds, call FreeMediaType to free the format block.
///////////////////////////////////////////////////////////////////////

inline HRESULT CreateRGBVideoType(AM_MEDIA_TYPE &mt, WORD iBitDepth, long Width, long Height,
	double fps)
{
	DWORD color_mask_565[] = { 0x00F800, 0x0007E0, 0x00001F };


	if ((iBitDepth != 1) && (iBitDepth != 4) && (iBitDepth != 8) &&
		(iBitDepth != 16) && (iBitDepth != 24) && (iBitDepth != 32))
	{
		return E_INVALIDARG;
	}

	if (Width < 0)
	{
		return E_INVALIDARG;
	}

	FreeMediaType(mt);

	mt.pbFormat = (BYTE*)CoTaskMemAlloc(sizeof(VIDEOINFO));
	if (!mt.pbFormat)
	{
		return E_OUTOFMEMORY;
	}
	mt.cbFormat = sizeof(VIDEOINFO);


	VIDEOINFO *pvi = (VIDEOINFO*)mt.pbFormat;
	ZeroMemory(pvi, sizeof(VIDEOINFO));

	pvi->AvgTimePerFrame = FramesPerSecToFrameLength(fps);

	BITMAPINFOHEADER *pBmi = &(pvi->bmiHeader);
	pBmi->biSize = sizeof(BITMAPINFOHEADER);
	pBmi->biWidth = Width;
	pBmi->biHeight = Height;
	pBmi->biPlanes = 1;
	pBmi->biBitCount = iBitDepth;

	if (iBitDepth == 16)
	{
		pBmi->biCompression = BI_BITFIELDS;
		memcpy(pvi->dwBitMasks, color_mask_565, sizeof(DWORD)* 3);
	}
	else
	{
		pBmi->biCompression = BI_RGB;
	}

	if (iBitDepth <= 8)
	{
		// Palettized format.
		pBmi->biClrUsed = PALETTE_ENTRIES(pvi);

		HDC hdc = GetDC(NULL);  // hdc for the current display.
		if (hdc == NULL)
		{
			FreeMediaType(mt);
			return HRESULT_FROM_WIN32(GetLastError());
		}
		GetSystemPaletteEntries(hdc, 0, pBmi->biClrUsed, (PALETTEENTRY*)pvi->bmiColors);

		ReleaseDC(NULL, hdc);
	}

	pvi->bmiHeader.biSizeImage = DIBSIZE(pvi->bmiHeader);

	mt.majortype == MEDIATYPE_Video;
	mt.subtype = FORMAT_VideoInfo;

	switch (iBitDepth)
	{
	case 1:
		mt.subtype = MEDIASUBTYPE_RGB1;
		break;
	case 4:
		mt.subtype = MEDIASUBTYPE_RGB4;
		break;
	case 8:
		mt.subtype = MEDIASUBTYPE_RGB8;
		break;
	case 16:
		mt.subtype = MEDIASUBTYPE_RGB565;
		break;
	case 24:
		mt.subtype = MEDIASUBTYPE_RGB24;
		break;
	case 32:
		mt.subtype = MEDIASUBTYPE_RGB32;
	}

	mt.lSampleSize = pvi->bmiHeader.biSizeImage;
	mt.bTemporalCompression = FALSE;
	mt.bFixedSizeSamples = TRUE;
	return S_OK;
}

inline HRESULT CreateYUVVideoType(AM_MEDIA_TYPE &mt, long Width, long Height, double fps)
{
	DWORD color_mask_565[] = { 0x00F800, 0x0007E0, 0x00001F };

	if (Width < 0)
	{
		return E_INVALIDARG;
	}

	FreeMediaType(mt);

	mt.pbFormat = (BYTE*)CoTaskMemAlloc(sizeof(VIDEOINFO));
	if (!mt.pbFormat)
	{
		return E_OUTOFMEMORY;
	}
	mt.cbFormat = sizeof(VIDEOINFO);


	VIDEOINFO *pvi = (VIDEOINFO*)mt.pbFormat;
	ZeroMemory(pvi, sizeof(VIDEOINFO));

	pvi->AvgTimePerFrame = FramesPerSecToFrameLength(fps);

	BITMAPINFOHEADER *pBmi = &(pvi->bmiHeader);
	pBmi->biSize = sizeof(BITMAPINFOHEADER);
	pBmi->biWidth = Width;
	pBmi->biHeight = Height;
	pBmi->biPlanes = 1;
	pBmi->biBitCount = 24;

	pBmi->biCompression = BI_RGB;


	pvi->bmiHeader.biSizeImage = DIBSIZE(pvi->bmiHeader);

	mt.majortype == MEDIATYPE_Video;
	mt.subtype = FORMAT_VideoInfo;
	mt.subtype = MEDIASUBTYPE_YV12;
	mt.lSampleSize = pvi->bmiHeader.biSizeImage;
	mt.bTemporalCompression = FALSE;
	mt.bFixedSizeSamples = TRUE;
	return S_OK;
}

CFMPEGOutputPinBase::CFMPEGOutputPinBase(
	__in_opt LPCTSTR pObjectName,
	__in CBaseFilter *pFilter,
	__in CCritSec *pLock,
	__inout HRESULT *phr,
	__in_opt LPCWSTR pName) :CBaseOutputPin(pObjectName, pFilter, pLock, phr, pName)
{
	//*phr = pFilter->AddPin(this);
	m_pLock = pLock;	
}


CFMPEGOutputPinBase::~CFMPEGOutputPinBase()
{
}

// Active
//
// The pin is active - start up the worker thread
HRESULT CFMPEGOutputPinBase::Active(void) {

	CAutoLock lock(m_pLock);

	HRESULT hr;

	if (m_pFilter->IsActive()) {
		return S_FALSE;	// succeeded, but did not allocate resources (they already exist...)
	}

	// do nothing if not connected - its ok not to connect to
	// all pins of a source filter
	if (!IsConnected()) {
		return NOERROR;
	}

	hr = CBaseOutputPin::Active();
	if (FAILED(hr)) {
		return hr;
	}

	ASSERT(!ThreadExists());

	// start the thread
	if (!Create()) {
		return E_FAIL;
	}

	// Tell thread to initialize. If OnThreadCreate Fails, so does this.
	hr = Init();
	if (FAILED(hr))
		return hr;

	return Pause();
}


//
// Inactive
//
// Pin is inactive - shut down the worker thread
// Waits for the worker to exit before returning.
HRESULT CFMPEGOutputPinBase::Inactive(void) {

	CAutoLock lock(m_pLock);

	HRESULT hr;

	// do nothing if not connected - its ok not to connect to
	// all pins of a source filter
	if (!IsConnected()) {
		return NOERROR;
	}

	// !!! need to do this before trying to stop the thread, because
	// we may be stuck waiting for our own allocator!!!

	hr = CBaseOutputPin::Inactive();  // call this first to Decommit the allocator
	if (FAILED(hr)) {
		return hr;
	}

	if (ThreadExists()) {
		hr = Stop();

		if (FAILED(hr)) {
			return hr;
		}

		hr = Exit();
		if (FAILED(hr)) {
			return hr;
		}

		Close();	// Wait for the thread to exit, then tidy up.
	}

	// hr = CBaseOutputPin::Inactive();  // call this first to Decommit the allocator
	//if (FAILED(hr)) {
	//	return hr;
	//}

	return NOERROR;
}

//
// ThreadProc
//
// When this returns the thread exits
// Return codes > 0 indicate an error occured
DWORD CFMPEGOutputPinBase::ThreadProc(void) {

	HRESULT hr;  // the return code from calls
	Command com;

	do {
		com = GetRequest();
		if (com != CMD_INIT) {
			DbgLog((LOG_ERROR, 1, TEXT("Thread expected init command")));
			Reply((DWORD)E_UNEXPECTED);
		}
	} while (com != CMD_INIT);

	DbgLog((LOG_TRACE, 1, TEXT("CSourceStream worker thread initializing")));

	hr = OnThreadCreate(); // perform set up tasks
	if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 1, TEXT("CSourceStream::OnThreadCreate failed. Aborting thread.")));
		OnThreadDestroy();
		Reply(hr);	// send failed return code from OnThreadCreate
		return 1;
	}

	// Initialisation suceeded
	Reply(NOERROR);

	Command cmd;
	do {
		cmd = GetRequest();

		switch (cmd) {

		case CMD_EXIT:
			Reply(NOERROR);
			break;

		case CMD_RUN:
			DbgLog((LOG_ERROR, 1, TEXT("CMD_RUN received before a CMD_PAUSE???")));
			// !!! fall through???

		case CMD_PAUSE:
			Reply(NOERROR);
			DoBufferProcessingLoop();
			break;

		case CMD_STOP:
			Reply(NOERROR);
			break;

		default:
			DbgLog((LOG_ERROR, 1, TEXT("Unknown command %d received!"), cmd));
			Reply((DWORD)E_NOTIMPL);
			break;
		}
	} while (cmd != CMD_EXIT);

	hr = OnThreadDestroy();	// tidy up.
	if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 1, TEXT("CSourceStream::OnThreadDestroy failed. Exiting thread.")));
		return 1;
	}

	DbgLog((LOG_TRACE, 1, TEXT("CSourceStream worker thread exiting")));
	return 0;
}


//
// DoBufferProcessingLoop
//
// Grabs a buffer and calls the users processing function.
// Overridable, so that different delivery styles can be catered for.
HRESULT CFMPEGOutputPinBase::DoBufferProcessingLoop(void) {

	Command com;

	OnThreadStartPlay();

	do {
		while (!CheckRequest(&com)) {

			IMediaSample *pSample;

			HRESULT hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);
			if (FAILED(hr)) {
				Sleep(1);
				continue;	// go round again. Perhaps the error will go away
				// or the allocator is decommited & we will be asked to
				// exit soon.
			}

			// Virtual function user will override.
			hr = FillBuffer(pSample);

			if (hr == S_OK) {
				hr = Deliver(pSample);
				pSample->Release();

				// downstream filter returns S_FALSE if it wants us to
				// stop or an error if it's reporting an error.
				if (hr != S_OK)
				{
					DbgLog((LOG_TRACE, 2, TEXT("Deliver() returned %08x; stopping"), hr));
					return S_OK;
				}

			}
			else if (hr == S_FALSE) {
				// derived class wants us to stop pushing data
				pSample->Release();
				DeliverEndOfStream();
				return S_OK;
			}
			else {
				// derived class encountered an error
				pSample->Release();
				DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from FillBuffer!!!"), hr));
				DeliverEndOfStream();
				m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
				return hr;
			}

			// all paths release the sample
		}

		// For all commands sent to us there must be a Reply call!

		if (com == CMD_RUN || com == CMD_PAUSE) {
			Reply(NOERROR);
		}
		else if (com != CMD_STOP) {
			Reply((DWORD)E_UNEXPECTED);
			DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
		}
	} while (com != CMD_STOP);

	return S_FALSE;
}

//
// CheckMediaType
//
// Do we support this type? Provides the default support for 1 type.
HRESULT CFMPEGOutputPinBase::CheckMediaType(const CMediaType *pMediaType) {

	CAutoLock lock(m_pLock);

	CMediaType mt;
	GetMediaType(&mt);

	if (mt == *pMediaType) {
		return NOERROR;
	}

	return E_FAIL;
}


//
// GetMediaType/3
//
// By default we support only one type
// iPosition indexes are 0-n
HRESULT CFMPEGOutputPinBase::GetMediaType(int iPosition, __inout CMediaType *pMediaType) {

	CAutoLock lock(m_pLock);

	if (iPosition<0) {
		return E_INVALIDARG;
	}
	if (iPosition>1) {
		return VFW_S_NO_MORE_ITEMS;
	}
	return GetMediaType(pMediaType);
}

//
// Set Id to point to a CoTaskMemAlloc'd
STDMETHODIMP CFMPEGOutputPinBase::QueryId(__deref_out LPWSTR *Id) {
	CheckPointer(Id, E_POINTER);
	ValidateReadWritePtr(Id, sizeof(LPWSTR));

	// We give the pins id's which are 1,2,...
	// FindPinNumber returns -1 for an invalid pin
	int i = 1 + ((CXiaozaiPushFilter*)m_pFilter)->FindPinNumber(this);
	if (i<1) return VFW_E_NOT_FOUND;
	*Id = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR)* 12);
	if (*Id == NULL) {
		return E_OUTOFMEMORY;
	}
	IntToWstr(i, *Id);
	return NOERROR;
}

CXiaozaiPushFilterAudioOutputPin::CXiaozaiPushFilterAudioOutputPin(CXiaozaiPushFilterImp *pSUPeR,
	__in CBaseFilter *pFilter,
	__in CCritSec *pLock,
	__inout HRESULT *phr,
	__in_opt LPCWSTR pName) 
	:CFMPEGOutputPinBase(NAME("CXiaozaiPushFilterAudioOutputPin"), pFilter, pLock, phr, pName),
	m_pSUPeR(pSUPeR)
{

}

HRESULT CXiaozaiPushFilterAudioOutputPin::GetMediaType(__inout CMediaType *pMediaType)
{
	HRESULT hr = S_OK;
	*pMediaType = m_mt;
    return hr;
}

HRESULT CXiaozaiPushFilterAudioOutputPin::FillBuffer(IMediaSample *pSamp)
{
	CFMPEGOutputPinBase::FillBuffer(pSamp);
	return m_pSUPeR->FillMediaSample(pSamp, false);
}

HRESULT CXiaozaiPushFilterAudioOutputPin::SetAudioFormat(WORD nChannels, DWORD nSamplePersec, WORD wBitsPerSample)
{
	m_nChannels = nChannels;
	m_nSamplePersec = nSamplePersec;
	m_wBitsPerSample = wBitsPerSample;
	return CreatePCMAudioType(m_mt, nChannels, nSamplePersec, wBitsPerSample);
}

//-----------------------------------------------------------------------------
// Name: DecideBufferSize
// Desc: This will always be called after the format has been sucessfully
//		 negotiated. So we have a look at m_mt to see what size image we agreed.
//		 Then we can ask for buffers of the correct size to contain them.
//-----------------------------------------------------------------------------
HRESULT CXiaozaiPushFilterAudioOutputPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	CAutoLock cAutoLock(m_pLock);
	ASSERT(pAlloc);
	ASSERT(pProperties);
	HRESULT hr = NOERROR;

	VIDEOINFO *pvi = (VIDEOINFO *)m_mt.Format();
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_nChannels*m_nSamplePersec*m_wBitsPerSample*10/8;

	ASSERT(pProperties->cbBuffer);

	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties, &Actual);
	if (FAILED(hr)) {
		return hr;
	}

	// Is this allocator unsuitable

	if (Actual.cbBuffer < pProperties->cbBuffer) {
		return E_FAIL;
	}

	// Make sure that we have only 1 buffer (we erase the ball in the
	// old buffer to save having to zero a 200k+ buffer every time
	// we draw a frame)

	ASSERT(Actual.cBuffers == 1);
	return NOERROR;
}


CXiaozaiPushFilterVideoOutputPin::CXiaozaiPushFilterVideoOutputPin(CXiaozaiPushFilterImp *pSUPeR,
	__in CBaseFilter *pFilter,
	__in CCritSec *pLock,
	__inout HRESULT *phr,
	__in_opt LPCWSTR pName)
	:CFMPEGOutputPinBase(NAME("CXiaozaiPushFilterVideoOutputPin"), pFilter, pLock, phr, pName),
	m_pSUPeR(pSUPeR)
{

}

HRESULT CXiaozaiPushFilterVideoOutputPin::GetMediaType(__inout CMediaType *pMediaType)
{
	HRESULT hr = S_OK;
	*pMediaType = m_mt;
	return hr;
}

HRESULT CXiaozaiPushFilterVideoOutputPin::FillBuffer(IMediaSample *pSamp)
{
	CFMPEGOutputPinBase::FillBuffer(pSamp);
	return m_pSUPeR->FillMediaSample(pSamp, true);
}

HRESULT CXiaozaiPushFilterVideoOutputPin::SetRGBFormat(WORD iBitDepth, long Width, long Height, double fps)
{
	return CreateRGBVideoType(m_mt,iBitDepth,Width,Height,fps);
}

HRESULT CXiaozaiPushFilterVideoOutputPin::SetYUVFormat(long Width, long Height, double fps)
{
	m_nWidth = Width;
	m_nHeight = Height;
	return CreateYUVVideoType(m_mt, Width, Height, fps);
}

//-----------------------------------------------------------------------------
// Name: DecideBufferSize
// Desc: This will always be called after the format has been sucessfully
//		 negotiated. So we have a look at m_mt to see what size image we agreed.
//		 Then we can ask for buffers of the correct size to contain them.
//-----------------------------------------------------------------------------
HRESULT CXiaozaiPushFilterVideoOutputPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	CAutoLock cAutoLock(m_pLock);
	ASSERT(pAlloc);
	ASSERT(pProperties);
	HRESULT hr = NOERROR;

	VIDEOINFO *pvi = (VIDEOINFO *)m_mt.Format();
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_nWidth*m_nHeight*3;

	ASSERT(pProperties->cbBuffer);

	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties, &Actual);
	if (FAILED(hr)) {
		return hr;
	}

	// Is this allocator unsuitable

	if (Actual.cbBuffer < pProperties->cbBuffer) {
		return E_FAIL;
	}

	// Make sure that we have only 1 buffer (we erase the ball in the
	// old buffer to save having to zero a 200k+ buffer every time
	// we draw a frame)

	ASSERT(Actual.cBuffers == 1);
	return NOERROR;
}
