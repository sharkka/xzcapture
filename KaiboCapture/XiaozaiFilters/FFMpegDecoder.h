#pragma once
extern "C"{
#include "libavcodec\avcodec.h"
#include "libswscale\swscale.h"
#include "libavutil\imgutils.h"
#include "libswresample/swresample.h"
}
#include "map"
#include <objidl.h>
#include <gdiplus.h>
#include "ComHelper.h"
#include "dshow.h"

using namespace Gdiplus;


class FFMpegDecoder
{
protected:
	AVCodecContext * m_pCodecContext = NULL;
	AVCodec *  m_pCodec = NULL;
	//the option of the codec
	AVDictionary *m_Options = NULL;
	bool m_isOpened = false;
	int m_classType;

public:
	FFMpegDecoder();
	virtual ~FFMpegDecoder();
	virtual bool open_codec(AVCodecContext * codecContext);
	virtual bool open_codec(const char * codec_name);
	virtual bool open_codec(enum AVCodecID id);

	int IsH264()
	{
		if (m_isOpened)
		{
			if ((m_pCodec->id == AV_CODEC_ID_H264))
				return 1;
			else
				return 0;
		}
		else
			return -1;
	}

	int IsAAC()
	{		
		if (m_isOpened)
		{
			if ((m_pCodec->id == AV_CODEC_ID_AAC))
				return 1;
			else
				return 0;
		}
		else
			return -1;
	}

	bool IsOpen(){ return m_isOpened; };
	///
	/// set the options of the codec, should be called before open
	///	 
	bool set_options(std::map<const char*, const char *> optionmap, int flags);
	virtual bool set_options(BSTR argv, int flags) = 0;

	AVCodecContext * get_codec_cxt(){ return m_pCodecContext; };
};

class FFMpegVideoDecoder : public FFMpegDecoder
{
	//the decoded image, YUV
	AVFrame* m_pOutFrame = NULL;

	//the scaled/filtered image 
	AVFrame* m_pOutFrame2 = NULL;
	int m_iOutWidth = -1;
	int m_iOutHeight = -1;
	AVPixelFormat m_dstFormat = AVPixelFormat::AV_PIX_FMT_NONE;

	//
	SwsContext * m_SWSContext = NULL;

	//used for BMP output
	BITMAPINFO * m_pBmpInfo = NULL;
	void * m_pBmpData = NULL;

	//Scale the image and convert the format
	//Note: need to use av_frame_unref to release the frame
	AVFrame* scale(int dstWidth, int dstHeight, AVPixelFormat dstFormat);

public:
	FFMpegVideoDecoder(){ m_classType = 0; };
	virtual ~FFMpegVideoDecoder();

	//the caller of this function does not need to free the returned AVFrame*
	//Note: need to use av_frame_unref to release the frame
	AVFrame* decode(AVPacket* pPacket);

	///
	///Decode a frame, and then scale it to the destinated size, convert to the dstFormat
	///Note: need to use av_frame_unref to release the frame
	AVFrame* decode_with_scale(AVPacket* pPacket, int dstWidth, int dstHeight, AVPixelFormat dstFormat);

	///Decode a frame, and then scale it to the destinated size, convert to a GDI+ BMP
	///note: the width of BMP must be 4 aligned
	///the Bitmap should be deleted by the caller, but its content will be managed by 
	// 
	Bitmap * decode2BMP(AVPacket* pPacket, int dstWidth, int dstHeight);

	bool set_options(BSTR argv, int flags);

	void close_codec() {
		if (m_pCodecContext) {
			avcodec_close(m_pCodecContext);
			m_pCodecContext = NULL;
		}
		if (m_pOutFrame)
			av_frame_free(&m_pOutFrame);
		m_pOutFrame = NULL;

		if (m_pOutFrame2)
		{
			if (m_pOutFrame2->data[0])
				av_freep(m_pOutFrame2->data[0]);
			av_frame_free(&m_pOutFrame2);
		}
		m_pOutFrame2 = NULL;
	}

};


class FFMpegAudioDecoder : public FFMpegDecoder
{
	//the decoded Audio
	AVFrame* m_pOutFrame = NULL;

	//the resampled audio 
	AVFrame* m_pOutFrame2 = NULL;
	SwrContext *m_SwrContext = NULL;
	AVSampleFormat m_dstFormat = AVSampleFormat::AV_SAMPLE_FMT_NONE;
	int64_t m_dst_ch_layout = 0;
	int m_dst_sample_rate = 0;

	AVFrame* resample(int64_t out_ch_layout, AVSampleFormat out_sample_fmt, int out_sample_rate);

public:
	FFMpegAudioDecoder(){ m_classType = 1; };
	virtual ~FFMpegAudioDecoder();

	//Note: need to use av_frame_unref to release the frame
	AVFrame* decode(AVPacket* pPacket);

	inline int set_request_fmt(AVSampleFormat out_sample_fmt, uint64_t channel_layout)
	{
		if (!m_isOpened)
			return -1;
		m_pCodecContext->request_channel_layout = channel_layout;
		m_pCodecContext->request_sample_fmt = out_sample_fmt;
		return 0;
	}
	///
	///Decode a frame, and then resample it to the destinated size, convert to the dstFormat
	///Note: need to use av_frame_unref to release the frame
	AVFrame* decode_with_resample(AVPacket* pPacket, int64_t out_ch_layout, AVSampleFormat out_sample_fmt, int out_sample_rate);

	bool set_options(BSTR argv, int flags);

	void close_codec() {
		if (m_pCodecContext) {
			avcodec_close(m_pCodecContext);
			m_pCodecContext = NULL;
		}
		if (m_pOutFrame)
			av_frame_free(&m_pOutFrame);
		m_pOutFrame = NULL;
	}
};