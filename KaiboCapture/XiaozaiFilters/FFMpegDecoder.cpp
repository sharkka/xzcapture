#include "stdafx.h"
#include "list"
#include "string"
#include "Win32_Utils.h"
#include "FFMpegDecoder.h"


#pragma comment (lib,"Gdiplus.lib")

int SplitArgs(BSTR argv, _tstring argstrs[3]);
int ParseOptions(AVDictionary **options, _tstring argv, int flags);

FFMpegDecoder::FFMpegDecoder()
{
	
}


FFMpegDecoder::~FFMpegDecoder()
{
	if (m_pCodecContext) {
		avcodec_close(m_pCodecContext);
		m_pCodecContext = NULL;
	}
	if (m_Options)
	{
		av_dict_free(&m_Options);
		m_Options = NULL;
	}
}


bool FFMpegDecoder::open_codec(AVCodecContext * codecContext)
{
	m_pCodecContext = codecContext;
	if (m_pCodecContext == NULL)
		return false;
	m_pCodec = avcodec_find_decoder(m_pCodecContext->codec_id);
	if (m_pCodec == NULL){
		return false;
	}
	if (avcodec_open2(m_pCodecContext, m_pCodec, &m_Options)<0){
		return false;
	}
	m_isOpened = true;
	return true;
}

bool FFMpegDecoder::open_codec(const char * codec_name)
{	
	m_pCodec = avcodec_find_decoder_by_name(codec_name);
	if (m_pCodec == NULL){
		return false;
	}
	if (m_pCodecContext == NULL)
		m_pCodecContext = avcodec_alloc_context3(m_pCodec);

	if (avcodec_open2(m_pCodecContext, m_pCodec, &m_Options)<0){
		return false;
	}
	m_isOpened = true;
	return true;
}

bool FFMpegDecoder::open_codec(enum AVCodecID id)
{
	m_pCodec = avcodec_find_decoder(id);
	if (m_pCodec == NULL){
		return false;
	}
	if (m_pCodecContext == NULL)
		m_pCodecContext = avcodec_alloc_context3(m_pCodec);

	if (avcodec_open2(m_pCodecContext, m_pCodec, &m_Options)<0){
		return false;
	}
	m_isOpened = true;
	return true;
}

bool FFMpegDecoder::set_options(std::map<const char*, const char *> optionmap, int flags)
{
	int ret;
	for each (std::pair<const char*, const char *> item in optionmap)
	{
		ret = av_dict_set(&m_Options, item.first, item.second, flags);
		if (ret < 0)
			return false;
	}
	return true;
}




FFMpegVideoDecoder::~FFMpegVideoDecoder()
{
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

	if (m_SWSContext)
		sws_freeContext(m_SWSContext);
	m_SWSContext = NULL;

	if (m_Options)
	{
		av_dict_free(&m_Options);
		m_Options = NULL;
	}
	SAFE_DELETE(m_pBmpData);
	SAFE_DELETE(m_pBmpInfo);
}

AVFrame* FFMpegVideoDecoder::decode(AVPacket* pPacket)
{
	int got_picture;
	if (m_pOutFrame == NULL)
		m_pOutFrame = av_frame_alloc();
	int ret = avcodec_decode_video2(m_pCodecContext, m_pOutFrame, &got_picture, pPacket);
	m_pOutFrame->pts = av_frame_get_best_effort_timestamp(m_pOutFrame);
	if ((ret < 0) || got_picture == 0) {
		printf("#####av_frame_get_best_effort_timestamp return %ld, got_picture=%d, decode codec id: %d, frame codecid: %d\n",
			m_pOutFrame->pts, got_picture, m_pCodecContext->codec_id, m_pOutFrame->format);
		return NULL;
	}
	//printf("@@@@@av_frame_get_best_effort_timestamp return %ld, got_picture=%d, decode codec id: %d, frame codecid: %d\n",
	//	m_pOutFrame->pts, got_picture, m_pCodecContext->codec_id, m_pOutFrame->format);
	return m_pOutFrame;
}

bool FFMpegVideoDecoder::set_options(BSTR argv, int flags)
{
	_tstring argstrs[3] = { _T(""), _T(""), _T("") };
	SplitArgs(argv, argstrs);
	bool ret = (ParseOptions(&m_Options, argstrs[1], flags) >= 0);
	return ret;
};

AVFrame* FFMpegVideoDecoder::decode_with_scale(AVPacket* pPacket, int dstWidth, int dstHeight, AVPixelFormat dstFormat)
{
	AVFrame* decodedimg = decode(pPacket);
	if (decodedimg == NULL)
		return NULL;	

	AVFrame* res = scale(dstWidth, dstHeight, dstFormat);
	
	//released the decoded buf
	if (decodedimg != res)
		av_frame_unref(decodedimg);
	return res;
}


AVFrame* FFMpegVideoDecoder::scale(int dstWidth, int dstHeight, AVPixelFormat dstFormat)
{
	dstWidth = (dstWidth == 0) ? m_pCodecContext->width : dstWidth;
	dstHeight = (dstHeight == 0) ? m_pCodecContext->height : dstHeight;

	if ((m_pOutFrame->width == dstWidth) && (m_pOutFrame->height == dstHeight) && (m_pOutFrame->format == dstFormat))
		return m_pOutFrame;
	//Check whether the size has been changed
	if (m_iOutWidth != -1 && m_iOutHeight != -1 && (m_iOutWidth != dstWidth || m_iOutHeight != dstHeight || m_dstFormat != dstFormat))
	{
		if (m_SWSContext)
			sws_freeContext(m_SWSContext);
		m_SWSContext = NULL;
	}
	
	m_iOutHeight = dstHeight;
	m_iOutWidth = dstWidth;
	m_dstFormat = dstFormat;
	int ret = -1;

	if (m_pOutFrame2)
	{
		if (&m_pOutFrame2->data[0])
			av_freep(&m_pOutFrame2->data[0]);
		if (m_pOutFrame2) {
			av_frame_unref(m_pOutFrame2);
			av_frame_free(&m_pOutFrame2);
			m_pOutFrame2 = NULL;
		}
	}
	m_pOutFrame2 = av_frame_alloc();
	ret = av_image_alloc(m_pOutFrame2->data, m_pOutFrame2->linesize, dstWidth, dstHeight, dstFormat, 1);
	if (ret < 0)
		return NULL;
		
	if (m_SWSContext == NULL)
	{
		m_SWSContext = sws_getContext(m_pCodecContext->width, m_pCodecContext->height, m_pCodecContext->pix_fmt, dstWidth, dstHeight, dstFormat, SWS_BICUBIC, NULL, NULL, NULL);
	}

	av_frame_copy_props(m_pOutFrame2, m_pOutFrame);

	ret = sws_scale(m_SWSContext, (const uint8_t* const*)m_pOutFrame->data, m_pOutFrame->linesize, 0, m_pCodecContext->height, m_pOutFrame2->data, m_pOutFrame2->linesize);
	if (ret < 0)
		return NULL;
	if (!m_pOutFrame2->sample_aspect_ratio.num)    m_pOutFrame2->sample_aspect_ratio = m_pCodecContext->sample_aspect_ratio;
	if (!m_pOutFrame2->width)                      m_pOutFrame2->width = m_pCodecContext->width;
	if (!m_pOutFrame2->height)                     m_pOutFrame2->height = m_pCodecContext->height;
	if (m_pOutFrame2->format == AV_PIX_FMT_NONE)   m_pOutFrame2->format = dstFormat;
	
	return m_pOutFrame2;
}


Bitmap * FFMpegVideoDecoder::decode2BMP(AVPacket* pPacket, int dstWidth, int dstHeight)
{
	AVFrame* decodedimg = decode(pPacket);
	if (decodedimg == NULL)
		return NULL;
	AVFrame* tmpFrame = scale(dstWidth, dstHeight, AV_PIX_FMT_BGR24);
	//released the decoded buf
	if (decodedimg!=tmpFrame)
		av_frame_unref(decodedimg);

	if (tmpFrame == NULL)
		return NULL;

	long imgsize = abs(tmpFrame->linesize[0])*tmpFrame->height;
	int top_down = (tmpFrame->linesize[0] > 0)? 1:-1;

	//size is updated?
	if (m_pBmpData && m_pBmpInfo && ((m_pBmpInfo->bmiHeader.biWidth != tmpFrame->width) || (m_pBmpInfo->bmiHeader.biHeight != tmpFrame->height)))
	{
		delete m_pBmpData;
		m_pBmpData = NULL;
	}

	if (m_pBmpData == NULL)
		m_pBmpData = new BYTE[imgsize];

	if (m_pBmpInfo == NULL)
	{
		m_pBmpInfo = new BITMAPINFO;
		m_pBmpInfo->bmiHeader.biSize = 40;
		m_pBmpInfo->bmiHeader.biWidth = tmpFrame->width;
		m_pBmpInfo->bmiHeader.biHeight = tmpFrame->height; //note: we have a top-down bmp
		m_pBmpInfo->bmiHeader.biPlanes = 1;
		m_pBmpInfo->bmiHeader.biBitCount = 24;
		m_pBmpInfo->bmiHeader.biClrImportant = 0;
		m_pBmpInfo->bmiHeader.biClrUsed = 0;
		m_pBmpInfo->bmiHeader.biCompression = BI_RGB;
		m_pBmpInfo->bmiHeader.biSizeImage = 0;
		m_pBmpInfo->bmiHeader.biXPelsPerMeter = 0;
		m_pBmpInfo->bmiHeader.biYPelsPerMeter = 0;
	}
	else
	{
		m_pBmpInfo->bmiHeader.biWidth = tmpFrame->width;
		m_pBmpInfo->bmiHeader.biHeight = tmpFrame->height; //note: we have a top-down bmp
	}
	if (top_down > 0)
		memcpy_s(m_pBmpData, imgsize, tmpFrame->data[0], imgsize);
	else
	{
		BYTE* pSrc = tmpFrame->data[0] + tmpFrame->linesize[0] * (tmpFrame->height - 1);
		memcpy_s(m_pBmpData, imgsize, pSrc, imgsize);
	}
	Bitmap * mybmp = new Bitmap(m_pBmpInfo, m_pBmpData);
	av_frame_unref(tmpFrame);
	return mybmp;
}


FFMpegAudioDecoder::~FFMpegAudioDecoder()
{
	if (m_pCodecContext) {
		avcodec_close(m_pCodecContext);
		m_pCodecContext = NULL;
	}
	if (m_pOutFrame)
		av_frame_free(&m_pOutFrame);
	m_pOutFrame = NULL;

	if (m_SwrContext)
		swr_free(&m_SwrContext);
	m_SwrContext = NULL;
}

AVFrame* FFMpegAudioDecoder::decode(AVPacket* pPacket)
{
	int got_audio;
	if (m_pOutFrame == NULL)
		m_pOutFrame = av_frame_alloc();
	int ret = avcodec_decode_audio4(m_pCodecContext, m_pOutFrame, &got_audio, pPacket);
	if ((ret < 0) || got_audio == 0)
		return NULL;
	return m_pOutFrame;
}


AVFrame* FFMpegAudioDecoder::resample(int64_t out_ch_layout, AVSampleFormat out_sample_fmt, int out_sample_rate)
{
	out_sample_fmt = (out_sample_fmt == AV_SAMPLE_FMT_NONE)? m_pCodecContext->sample_fmt:out_sample_fmt;
	out_ch_layout = (out_ch_layout == 0) ? m_pCodecContext->channel_layout : out_ch_layout;
	out_sample_rate = (out_sample_rate == 0) ? m_pCodecContext->sample_rate : out_sample_rate;

	//
	if ((out_ch_layout == m_pCodecContext->channel_layout) && (out_sample_fmt == m_pCodecContext->sample_fmt) && (out_sample_rate == m_pCodecContext->sample_rate))
	{
		return m_pOutFrame;
	}

	if ((out_ch_layout != m_dst_ch_layout) || (out_sample_fmt != m_dstFormat) || (out_sample_rate != m_dst_sample_rate))
	{
		if (m_SwrContext)
			swr_free(&m_SwrContext);
		m_SwrContext = NULL;
		if (m_pOutFrame2)
			av_frame_free(&m_pOutFrame2);
		m_pOutFrame2 = NULL;
	}

	if (m_SwrContext == NULL)
	{
		m_SwrContext = swr_alloc();
		m_SwrContext = swr_alloc_set_opts(m_SwrContext, out_ch_layout, out_sample_fmt, out_sample_rate, m_pCodecContext->channel_layout, m_pCodecContext->sample_fmt, m_pCodecContext->sample_rate, 0, NULL);
		swr_init(m_SwrContext);
		m_dstFormat = out_sample_fmt;
		m_dst_ch_layout = out_ch_layout;
		m_dst_sample_rate = out_sample_rate;
	}
	if (m_pOutFrame2 == NULL)
	{
		m_pOutFrame2 = av_frame_alloc();
		m_pOutFrame2->sample_rate = out_sample_rate;
		m_pOutFrame2->channel_layout = out_ch_layout;
		m_pOutFrame2->format = out_sample_fmt;
		m_pOutFrame2->nb_samples = m_pCodecContext->frame_size;
		m_pOutFrame2->channels = m_pCodecContext->channels;
	}

	av_frame_copy_props(m_pOutFrame2, m_pOutFrame);

	int ret = swr_convert_frame(m_SwrContext, m_pOutFrame2, m_pOutFrame);
	if (ret != 0)
		return NULL;
	return m_pOutFrame2;
}




AVFrame* FFMpegAudioDecoder::decode_with_resample(AVPacket* pPacket, int64_t out_ch_layout, AVSampleFormat out_sample_fmt, int out_sample_rate)
{
	AVFrame* decodedaud = decode(pPacket);
	if (decodedaud == NULL)
		return NULL;

	AVFrame* res = resample( out_ch_layout,  out_sample_fmt,  out_sample_rate);

	//released the decoded buf
	if (res!=decodedaud)
		av_frame_unref(decodedaud);
	return res;
}

bool FFMpegAudioDecoder::set_options(BSTR argv, int flags)
{
	_tstring argstrs[3] = { _T(""), _T(""), _T("")};
	SplitArgs(argv, argstrs);
	bool ret = (ParseOptions(&m_Options, argstrs[2], flags) >= 0);
	return ret;
}