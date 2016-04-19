#include "stdafx.h"
#include "Config.h"

#include <commdlg.h>
#include "FilterGUIDs.h"

#include "dvdmedia.h"
#include <strsafe.h>

#include "Uploader.h"
#include "UploaderInputPins.h"




//
//  Definition of CFMPEGInputPinBase
//
CFMPEGInputPinBase::CFMPEGInputPinBase(CXiaozaiPushFilterImp *pSUPeR,
							__in_opt LPCTSTR pObjectName,
                             CBaseFilter *pFilter,
                             CCritSec *pLock,
                             CCritSec *pReceiveLock,
                             HRESULT *phr,
							 LPCTSTR pName
							 ) :
CRenderedInputPin(pObjectName,
                  pFilter,                   // Filter
                  pLock,                     // Locking
                  phr,                       // Return code
                  pName),                 // Pin name
    m_pReceiveLock(pReceiveLock),
    m_pSUPeR(pSUPeR),
    m_tFirstSample(0),
	m_tTotalDuration(0),
	m_bInit(FALSE),
	m_bVideo(TRUE)
{
}

//
// BreakConnect
//
// Break a connection
//
HRESULT CFMPEGInputPinBase::BreakConnect()
{
    if (m_pSUPeR->m_pPosition != NULL) {
        m_pSUPeR->m_pPosition->ForceRefresh();
    }

    return CRenderedInputPin::BreakConnect();
}


//
// ReceiveCanBlock
//
// We don't hold up source threads on Receive
//
STDMETHODIMP CFMPEGInputPinBase::ReceiveCanBlock()
{
    return S_OK;
}


//
// Receive
//
// Do something with this media sample
//
STDMETHODIMP CFMPEGInputPinBase::Receive(IMediaSample *pSample)
{
	HRESULT hr = S_OK;

    CheckPointer(pSample,E_POINTER);
	BYTE *pData = NULL;
	pSample->GetPointer(&pData);
	CheckPointer(pData,E_POINTER);

    //CAutoLock lock(m_pReceiveLock);

    REFERENCE_TIME tStart, tEnd;
    hr = pSample->GetTime(&tStart, &tEnd);
    if (hr == VFW_E_SAMPLE_TIME_NOT_SET)
    {
        DbgLog((LOG_TRACE, 0, TEXT("Sample no timestamp!")));
        hr = pSample->GetMediaTime(&tStart, &tEnd);
        if (hr == VFW_E_MEDIA_TIME_NOT_SET)
            return S_OK;
    }

	if (!m_bVideo) //if it is an audio, 
	{
		if (tStart > 400000000000000000LL) {
			/* initial frames sometimes start < 0 (shown as a very large number here,
			like 437650244077016960 which FFmpeg doesn't like.
			TODO figure out math. For now just drop them. */
			DbgLog((LOG_TRACE, 1, TEXT("dshow dropping initial (or ending) audio frame with odd PTS too high %d\n"), tStart));
			return S_OK;
		}
	}

    DbgLog((LOG_TRACE, 1, TEXT("tStart(%s), tStop(%s), Diff(%d ms), Bytes(%d)"),
           (LPCTSTR) CDisp(tStart),
           (LPCTSTR) CDisp(tEnd),
           (LONG)((m_tTotalDuration - (MILLISECONDS_TO_100NS_UNITS(GetTickCount()) - m_tFirstSample))/(UNITS/MILLISECONDS)),
           pSample->GetActualDataLength()));

    if(!m_bInit)
	{
		m_tFirstSample = MILLISECONDS_TO_100NS_UNITS(GetTickCount());
		m_tTotalDuration = 0;
        m_bInit = TRUE;
        m_tStart = tStart;
        DbgLog((LOG_TRACE, 0, TEXT("######## %s pin Fisrsample %I64i arrive at %I64i"), m_bVideo?TEXT("Video"):TEXT("Audio"),m_tStart, m_tFirstSample));
	}
	m_tTotalDuration = tStart - m_tStart;

    //Whenever it is compressed or not, we should check the timestamp always.
	//if(m_pSUPeR->IsCompressedData(m_bVideo?0:1)) 
	//{
		REFERENCE_TIME tNow = MILLISECONDS_TO_100NS_UNITS(GetTickCount());
		if(m_bInit && (((tNow - m_tFirstSample)+30000000) < m_tTotalDuration))
		{
			REFERENCE_TIME dwTimeToWait = (m_tTotalDuration - (tNow - m_tFirstSample)-30000000)/(UNITS/MILLISECONDS);
			DbgLog((LOG_TRACE, 2, TEXT("######## %s pin sleeps %d milliseconds at frame %I64i, now %I64i"), m_bVideo?TEXT("Video"):TEXT("Audio"), dwTimeToWait, tStart, tNow ));
			Sleep((DWORD)dwTimeToWait);
		}
	//}		

    hr = m_pSUPeR->Write(pSample, m_bVideo);
	return hr;
}


// do something with these media samples
STDMETHODIMP CFMPEGInputPinBase::ReceiveMultiple(__in_ecount(nSamples) IMediaSample **pSamples,	long nSamples,	__out long *nSamplesProcessed)
{
	long i;
	DbgLog((LOG_TRACE, 0, TEXT("ReceiveMultiple\n")));
	for (i = 0; i < nSamples; i++)
		Receive(pSamples[i]);

	*nSamplesProcessed = nSamples;
	return S_OK;
}
//
// EndOfStream
//
STDMETHODIMP CFMPEGInputPinBase::EndOfStream(void)
{
    CAutoLock lock(m_pReceiveLock);
    return CRenderedInputPin::EndOfStream();

} // EndOfStream


//
// NewSegment
//
// Called when we are seeked
//
STDMETHODIMP CFMPEGInputPinBase::NewSegment(REFERENCE_TIME tStart,
                                       REFERENCE_TIME tStop,
                                       double dRate)
{
	m_bInit = FALSE;
    m_tFirstSample = 0;
	m_tTotalDuration = 0;
    return S_OK;

} // NewSegment

//
// CheckMediaType
//
// Check if the pin can support this specific proposed type and format
//
HRESULT CXiaozaiPushFilterVideoInputPin::CheckMediaType(const CMediaType *pMt)
{
	HRESULT hr = S_OK;
    if (pMt->majortype != MEDIATYPE_Video)
    {        
        return S_FALSE;
    }
	if(pMt->formattype == FORMAT_VideoInfo)
	{
		if(pMt->subtype == MEDIASUBTYPE_RGB24 || pMt->subtype == WMMEDIASUBTYPE_WMV1 
			|| pMt->subtype == WMMEDIASUBTYPE_WMV2|| pMt->subtype == WMMEDIASUBTYPE_WMV3 || pMt->subtype == WMMEDIASUBTYPE_MP4S || pMt->subtype == WMMEDIASUBTYPE_MP43 \
            || pMt->subtype == WMMEDIASUBTYPE_M4S2|| pMt->subtype == WMMEDIASUBTYPE_WMVA || pMt->subtype == WMMEDIASUBTYPE_MPG4 || pMt->subtype == WMMEDIASUBTYPE_XVID \
            || pMt->subtype == WMMEDIASUBTYPE_RV40|| pMt->subtype == WMMEDIASUBTYPE_DIVX_MPEG4 ||pMt->subtype == WMMEDIASUBTYPE_H263 \
            || pMt->subtype == WMMEDIASUBTYPE_DIVX|| pMt->subtype == WMMEDIASUBTYPE_WVP2 || pMt->subtype == WMMEDIASUBTYPE_WMVP)
		{
            //Check the bitrate, we do not accept large bitrate for compressed video. for example 1M bitrate
            /*VIDEOINFOHEADER *phdr = reinterpret_cast<VIDEOINFOHEADER *>(pMt->Format());
            if((pMt->subtype != MEDIASUBTYPE_RGB24)&&(phdr->dwBitRate > 1024*1024))
                return S_FALSE;*/

            //Copy the media type
            if(this->m_pSUPeR->m_mtVideo.pbFormat!=NULL)
				FreeMediaType(this->m_pSUPeR->m_mtVideo);
			hr = CopyMediaType(&this->m_pSUPeR->m_mtVideo,pMt);
			VIDEOINFOHEADER *phdr = 
				reinterpret_cast<VIDEOINFOHEADER *>(this->m_pSUPeR->m_mtVideo.pbFormat);

			if(pMt->subtype == MEDIASUBTYPE_RGB24)
			{
				DWORD dwLen = phdr->bmiHeader.biSizeImage;
				if(dwLen == 0)
				{
					dwLen = phdr->bmiHeader.biHeight * phdr->bmiHeader.biWidth 
								* phdr->bmiHeader.biBitCount / 8;
				}
				SAFE_ARRAYDELETE(this->m_pSUPeR->m_pRawSnapshot);
				this->m_pSUPeR->m_pRawSnapshot = new BYTE[dwLen];
				this->m_pSUPeR->m_dwSnapshotLen = dwLen;
			}

            if (phdr->dwBitRate == 0)
            {
                if (phdr->AvgTimePerFrame)
                phdr->dwBitRate = (DWORD) (phdr->bmiHeader.biSizeImage * (80000000/phdr->AvgTimePerFrame));
                else
                    return S_FALSE;
                if (phdr->dwBitRate > 8000000)
                    phdr->dwBitRate = 8000000;
            }

            //Fill FCC
            if (pMt->subtype == WMMEDIASUBTYPE_WMV3)
                phdr->bmiHeader.biCompression = FCC_FORMAT_WMV3;
            else if(pMt->subtype == WMMEDIASUBTYPE_MP4S)
                phdr->bmiHeader.biCompression = FCC_FORMAT_MP4S;
            else if(pMt->subtype == WMMEDIASUBTYPE_WMV2)
                phdr->bmiHeader.biCompression = FCC_FORMAT_WMV2;
            else if(pMt->subtype == WMMEDIASUBTYPE_WMV1)
                phdr->bmiHeader.biCompression = FCC_FORMAT_WMV1;
            else if(pMt->subtype == WMMEDIASUBTYPE_MP43)
                phdr->bmiHeader.biCompression = FCC_FORMAT_MP43;
            else if(pMt->subtype == WMMEDIASUBTYPE_M4S2)
                phdr->bmiHeader.biCompression = FCC_FORMAT_M4S2;
            else if(pMt->subtype == WMMEDIASUBTYPE_WMVA)
                phdr->bmiHeader.biCompression = FCC_FORMAT_WMVA;
            else if(pMt->subtype == WMMEDIASUBTYPE_MPG4)
                phdr->bmiHeader.biCompression = FCC_FORMAT_MPG4;
            else if(pMt->subtype == WMMEDIASUBTYPE_XVID)
                phdr->bmiHeader.biCompression = FCC_FORMAT_XVID;
            else if(pMt->subtype == WMMEDIASUBTYPE_RV40)
                phdr->bmiHeader.biCompression = FCC_FORMAT_RV40;
            else if(pMt->subtype == WMMEDIASUBTYPE_DIVX_MPEG4)
                phdr->bmiHeader.biCompression = FCC_FORMAT_DIVX_MPEG4;
            else if(pMt->subtype == WMMEDIASUBTYPE_H263)
                phdr->bmiHeader.biCompression = FCC_FORMAT_H263;
            else if(pMt->subtype == WMMEDIASUBTYPE_DIVX)
                phdr->bmiHeader.biCompression = FCC_FORMAT_RCC_MPEG4;
            
			return S_OK;
		}
	}
	//else if(pMt->formattype == FORMAT_VideoInfo2)
	//{     // || pMt->subtype == WMMEDIASUBTYPE_WMV3
	//	if(pMt->subtype == MEDIASUBTYPE_RGB24 || pMt->subtype == WMMEDIASUBTYPE_MP4S || pMt->subtype == WMMEDIASUBTYPE_MP43 \
 //           || pMt->subtype == WMMEDIASUBTYPE_M4S2 || pMt->subtype == WMMEDIASUBTYPE_MPG4 || pMt->subtype == WMMEDIASUBTYPE_XVID)
	//	{
	//		VIDEOINFOHEADER2 *phdr0 = (VIDEOINFOHEADER2 *)pMt->pbFormat;
	//		if(phdr0->dwInterlaceFlags & AMINTERLACE_IsInterlaced 
	//			|| phdr0->dwControlFlags & AMCONTROL_USED )
	//		{
	//			return S_FALSE; 
	//		}
	//		if(this->m_pSUPeR->m_mtVideo.pbFormat!=NULL)
	//			FreeMediaType(this->m_pSUPeR->m_mtVideo);
	//		hr = CopyMediaType(&this->m_pSUPeR->m_mtVideo,pMt);

 //           //VIDEOINFOHEADER2 *phdr =  reinterpret_cast<VIDEOINFOHEADER2 *>(this->m_pSUPeR->m_mtVideo.pbFormat);
 //           //Rewrite the infoheader
 //           this->m_pSUPeR->m_mtVideo.SetFormatType(&FORMAT_VideoInfo);
 //           this->m_pSUPeR->m_mtVideo.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER));

 //           VIDEOINFOHEADER *phdr =  reinterpret_cast<VIDEOINFOHEADER *>(this->m_pSUPeR->m_mtVideo.pbFormat);

 //           phdr->rcSource = phdr0->rcSource;
 //           phdr->rcTarget = phdr0->rcTarget;
 //           phdr->dwBitRate = phdr0->dwBitRate;
 //           phdr->dwBitErrorRate = phdr0->dwBitErrorRate;
 //           phdr->AvgTimePerFrame = phdr0->AvgTimePerFrame;
 //           phdr->bmiHeader = phdr0->bmiHeader;

	//		
	//		DWORD dwLen = phdr->bmiHeader.biSizeImage;
	//		if(dwLen == 0)
	//		{
	//			dwLen = phdr->bmiHeader.biHeight * phdr->bmiHeader.biWidth 
	//				* phdr->bmiHeader.biBitCount / 8;
	//		}
	//		SAFE_ARRAYDELETE(this->m_pSUPeR->m_pRawSnapshot);
	//		this->m_pSUPeR->m_pRawSnapshot = new BYTE[dwLen];
	//		this->m_pSUPeR->m_dwSnapshotLen = dwLen;

 //           //Fill FCC
 //           if (pMt->subtype == WMMEDIASUBTYPE_WMV3)
 //               phdr->bmiHeader.biCompression = FCC_FORMAT_WMV3;
 //           else if(pMt->subtype == WMMEDIASUBTYPE_MP4S)
 //               phdr->bmiHeader.biCompression = FCC_FORMAT_MP4S;
 //           else if(pMt->subtype == WMMEDIASUBTYPE_WMV2)
 //               phdr->bmiHeader.biCompression = FCC_FORMAT_WMV2;
 //           else if(pMt->subtype == WMMEDIASUBTYPE_WMV1)
 //               phdr->bmiHeader.biCompression = FCC_FORMAT_WMV1;

	//		return S_OK;
	//	}
	//}
 //   else if (pMt->formattype == FORMAT_MPEGVideo)
 //   {
 //       if (pMt->subtype == MEDIASUBTYPE_MPEG1Payload)//MEDIASUBTYPE_MPEG1Packet)
 //       {
 //           if(this->m_pSUPeR->m_mtVideo.pbFormat!=NULL)
	//			FreeMediaType(this->m_pSUPeR->m_mtVideo);
	//		hr = CopyMediaType(&this->m_pSUPeR->m_mtVideo,pMt);
	//		MPEG1VIDEOINFO* mpghdr =  reinterpret_cast<MPEG1VIDEOINFO *>(this->m_pSUPeR->m_mtVideo.pbFormat);
 //           
	//		DWORD dwLen = mpghdr->hdr.bmiHeader.biSizeImage;

 //           //Fill the FCC
 //           mpghdr->hdr.bmiHeader.biCompression = FCC_FORMAT_MPEG1_PAYLOAD;

	//		if(dwLen == 0)
	//		{
	//			dwLen = mpghdr->hdr.bmiHeader.biHeight * mpghdr->hdr.bmiHeader.biWidth 
	//				* mpghdr->hdr.bmiHeader.biBitCount / 8;
	//		}
	//		SAFE_ARRAYDELETE(this->m_pSUPeR->m_pRawSnapshot);
	//		this->m_pSUPeR->m_pRawSnapshot = new BYTE[dwLen];
	//		this->m_pSUPeR->m_dwSnapshotLen = dwLen;
	//		return S_OK;
 //       }
 //   }

   // else if(pMt->formattype == FORMAT_MPEG2Video)
   // {
   //     if (pMt->subtype == WMMEDIASUBTYPE_MPEG2_VIDEO)
   //     {
   //         if(this->m_pSUPeR->m_mtVideo.pbFormat!=NULL)
			//	FreeMediaType(this->m_pSUPeR->m_mtVideo);
			//hr = CopyMediaType(&this->m_pSUPeR->m_mtVideo,pMt);
			//MPEG2VIDEOINFO* mpghdr =  reinterpret_cast<MPEG2VIDEOINFO *>(this->m_pSUPeR->m_mtVideo.pbFormat);
   //         
			//DWORD dwLen = mpghdr->hdr.bmiHeader.biSizeImage;

   //         //Fill the FCC
   //         mpghdr->hdr.bmiHeader.biCompression = FCC_FORMAT_MPEG2_VIDEO;

			//if(dwLen == 0)
			//{
			//	dwLen = mpghdr->hdr.bmiHeader.biHeight * mpghdr->hdr.bmiHeader.biWidth 
			//		* mpghdr->hdr.bmiHeader.biBitCount / 8;
			//}
			//SAFE_ARRAYDELETE(this->m_pSUPeR->m_pRawSnapshot);
			//this->m_pSUPeR->m_pRawSnapshot = new BYTE[dwLen];
			//this->m_pSUPeR->m_dwSnapshotLen = dwLen;
			//return S_OK;
   //     }
   // }
    return S_FALSE;
}

//
//  Definition of CXiaozaiPushFilterVideoInputPin
//
CXiaozaiPushFilterVideoInputPin::CXiaozaiPushFilterVideoInputPin(CXiaozaiPushFilterImp *pSUPeR,
                             LPUNKNOWN pUnk,
                             CBaseFilter *pFilter,
                             CCritSec *pLock,
                             CCritSec *pReceiveLock,
                             HRESULT *phr,
							 LPCTSTR pName
							 ) :
CFMPEGInputPinBase(pSUPeR,NAME("CXiaozaiPushFilterVideoInputPin"),pFilter,pLock,pReceiveLock,phr,pName)
{
	this->m_bVideo = TRUE;
}

//
// CheckMediaType
//
// Check if the pin can support this specific proposed type and format
//
HRESULT CXiaozaiPushFilterAudioInputPin::CheckMediaType(const CMediaType *pMt)
{
	HRESULT hr = S_OK;
    if (pMt->majortype != MEDIATYPE_Audio)
    {        
        return S_FALSE;
    }
	if(pMt->formattype == FORMAT_WaveFormatEx)   
	{
		if(pMt->subtype == MEDIASUBTYPE_PCM || pMt->subtype == WMMEDIASUBTYPE_WMAudioV9 \
			|| pMt->subtype == WMMEDIASUBTYPE_WMAudioV8 || pMt->subtype == WMMEDIASUBTYPE_XAID ) //|| pMt->subtype == WMMEDIASUBTYPE_COOK 
            //Jianguang LOU: pMt->subtype == WMMEDIASUBTYPE_MP3 || do not support MP3 because most of MP3 samples without timestamp
		{
			//WAVEFORMATEX *pwme = reinterpret_cast<WAVEFORMATEX *>(pMt->pbFormat);
			if(this->m_pSUPeR->m_mtAudio.pbFormat!=NULL)
				FreeMediaType(this->m_pSUPeR->m_mtAudio);
			hr = CopyMediaType(&this->m_pSUPeR->m_mtAudio,pMt);

            WAVEFORMATEX *pRecordWfe = reinterpret_cast<WAVEFORMATEX *>(this->m_pSUPeR->m_mtAudio.pbFormat);
            if (pMt->subtype == WMMEDIASUBTYPE_COOK)
                pRecordWfe->wFormatTag = FCC_FORMAT_COOK;                  

            if (pRecordWfe->wFormatTag != WAVE_FORMAT_PCM)
            {
                if (pRecordWfe->nAvgBytesPerSec == 0)
                {
                    hr = S_FALSE;
                }
            }
			return hr;
		}
	}

    return S_FALSE;
}

//
//  Definition of CXiaozaiPushFilterAudioInputPin
//
CXiaozaiPushFilterAudioInputPin::CXiaozaiPushFilterAudioInputPin(CXiaozaiPushFilterImp *pSUPeR,
                             LPUNKNOWN pUnk,
                             CBaseFilter *pFilter,
                             CCritSec *pLock,
                             CCritSec *pReceiveLock,
                             HRESULT *phr,
							 LPCTSTR pName
							 ) :
CFMPEGInputPinBase(pSUPeR,NAME("CXiaozaiPushFilterAudioInputPin"),pFilter,pLock,pReceiveLock,phr,pName)
{
	this->m_bVideo = FALSE;
}
