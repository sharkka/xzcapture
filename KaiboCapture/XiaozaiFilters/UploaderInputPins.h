#pragma once
class CFMPEGInputPinBase;
class CXiaozaiPushFilter;

//  Video FCC
#define FCC_FORMAT_MP4S          mmioFOURCC('M','P','4','S')
#define FCC_FORMAT_MP43          mmioFOURCC('M','P','4','3')
#define FCC_FORMAT_MPEG2_VIDEO   mmioFOURCC(0x26,0x80,0x6d,0xe0)
#define FCC_FORMAT_MPEG1_PAYLOAD mmioFOURCC(0x81,0xEB,0x36,0xe4)
#define FCC_FORMAT_WMV3          mmioFOURCC('W','M','V','3')
#define FCC_FORMAT_WMV2          mmioFOURCC('W','M','V','2')
#define FCC_FORMAT_WMV1          mmioFOURCC('W','M','V','1')
#define FCC_FORMAT_DIVX_MPEG4   mmioFOURCC('D','X','5','0')
#define FCC_FORMAT_WIS_MPEG4    mmioFOURCC('W','M','P',0x04)
#define FCC_FORMAT_SIGMA_MPEG4  mmioFOURCC('R','M','P','4')
#define FCC_FORMAT_H263         mmioFOURCC('H','2','6','3')
#define FCC_FORMAT_MOTION_JPEG  mmioFOURCC('M','J','P','G')
#define FCC_FORMAT_RCC_MPEG4    mmioFOURCC('D','I','V','X')
#define FCC_FORMAT_MPG4         mmioFOURCC('M','P','G','4')
#define FCC_FORMAT_XVID         mmioFOURCC('X','V','I','D')
#define FCC_FORMAT_RV40         mmioFOURCC('R','V','4','0')
#define FCC_FORMAT_M4S2         mmioFOURCC('M','4','S','2')
#define FCC_FORMAT_WMVA         0x41564D57


//Audio FCC
#define FCC_FORMAT_XAID         0x2000
#define FCC_FORMAT_COOK         0x4F43

class CXiaozaiPushFilterImp;

class CFMPEGInputPinBase : public CRenderedInputPin
{
protected:
    CXiaozaiPushFilterImp *		const m_pSUPeR;         // Main renderer object
    CCritSec *		const m_pReceiveLock;	// Sample critical section
	BOOL			m_bVideo;				// is this pin video pin?
    REFERENCE_TIME	m_tFirstSample;			// Last sample receive time (system time)
    REFERENCE_TIME	m_tTotalDuration;		// Last sample's duration time (system time)
    REFERENCE_TIME  m_tStart;               // The first frame's timestamp
	BOOL			m_bInit;				// Initial or not


public:

	CFMPEGInputPinBase(CXiaozaiPushFilterImp *pSUPeR,
				__in_opt LPCTSTR pObjectName,
                  CBaseFilter *pFilter,
                  CCritSec *pLock,
                  CCritSec *pReceiveLock,
                  HRESULT *phr,
				  LPCTSTR pName);

    // Do something with this media sample
    STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP EndOfStream(void);
    STDMETHODIMP ReceiveCanBlock();
	// do something with these media samples
	STDMETHODIMP ReceiveMultiple(
		__in_ecount(nSamples) IMediaSample **pSamples,
		long nSamples,
		__out long *nSamplesProcessed);

    // Check if the pin can support this specific proposed type and format
    HRESULT virtual CheckMediaType(const CMediaType *)=0;

    // Break connection
    HRESULT BreakConnect();

    // Track NewSegment
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart,
                            REFERENCE_TIME tStop,
                            double dRate);
};

class CXiaozaiPushFilterVideoInputPin : public CFMPEGInputPinBase
{
public:
	CXiaozaiPushFilterVideoInputPin(CXiaozaiPushFilterImp *pSUPeR,
						  LPUNKNOWN pUnk,
						  CBaseFilter *pFilter,
						  CCritSec *pLock,
						  CCritSec *pReceiveLock,
						  HRESULT *phr,
						  LPCTSTR pName);
    // Check if the pin can support this specific proposed type and format
    HRESULT virtual CheckMediaType(const CMediaType *);
};

class CXiaozaiPushFilterAudioInputPin : public CFMPEGInputPinBase
{
public:
    CXiaozaiPushFilterAudioInputPin(CXiaozaiPushFilterImp *pSUPeR,
						  LPUNKNOWN pUnk,
						  CBaseFilter *pFilter,
						  CCritSec *pLock,
						  CCritSec *pReceiveLock,
						  HRESULT *phr,
						  LPCTSTR pName);
    // Check if the pin can support this specific proposed type and format
    HRESULT virtual CheckMediaType(const CMediaType *);
};
