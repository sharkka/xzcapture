#include "stdafx.h"
extern "C"{
#include "libavcodec\avcodec.h"
#include "libswscale\swscale.h"
#include "libavutil\imgutils.h"
}
#include <map>
#include <objidl.h>
#include "FFMpegEncoder.h"
#include "FFMpegOutputWapper.h"
#pragma warning(disable:4010 4244)

int SplitArgs(BSTR argv, _tstring argstrs[3]);
int ParseOptions(AVDictionary **options, _tstring argv, int flags);

FFMpegEncoder::FFMpegEncoder()
{
}


FFMpegEncoder::~FFMpegEncoder()
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

bool FFMpegEncoder::open_codec(AVCodecContext * codecContext)
{
	m_pCodecContext = codecContext;
	if (m_pCodecContext == NULL)
		return false;
	m_pCodec = avcodec_find_encoder(m_pCodecContext->codec_id);
	if (m_pCodec == NULL){
		return false;
	}
	m_pCodecContext->log_level_offset = AV_LOG_VERBOSE;
	int ret = avcodec_open2(m_pCodecContext, m_pCodec, &m_Options);
	if (ret<0){
		return false;
	}
	samples_index = 0;
	m_isOpened = true;
	
	return true;
}

bool FFMpegEncoder::open_codec(const char * codec_name)
{
	m_pCodec = avcodec_find_encoder_by_name(codec_name);
	if (m_pCodec == NULL){
		return false;
	}
	if (m_pCodecContext == NULL)
		m_pCodecContext = avcodec_alloc_context3(m_pCodec);

	if (avcodec_open2(m_pCodecContext, m_pCodec, &m_Options)<0){
		return false;
	}
	samples_index = 0;
	m_isOpened = true;
	return true;
}

bool FFMpegEncoder::open_codec(enum AVCodecID id)
{
	m_pCodec = avcodec_find_encoder(id);
	if (m_pCodec == NULL){
		return false;
	}
	if (m_pCodecContext == NULL)
		m_pCodecContext = avcodec_alloc_context3(m_pCodec);

	if (avcodec_open2(m_pCodecContext, m_pCodec, &m_Options)<0){
		return false;
	}
	samples_index = 0;
	m_isOpened = true;
	return true;
}

bool FFMpegEncoder::set_options(std::map<const char*, const char *> optionmap, int flags)
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

bool FFMpegAudioEncoder::set_options(BSTR argv, int flags)
{
	_tstring argstrs[3] = { _T(""), _T(""), _T("") };
	SplitArgs(argv, argstrs);
	bool ret = (ParseOptions(&m_Options, argstrs[2], flags) >= 0);
	return ret;
}

FFMpegAudioEncoder::FFMpegAudioEncoder()
{
	memset(&pkt, 0, sizeof(pkt));
}


FFMpegAudioEncoder::~FFMpegAudioEncoder()
{
	if (m_pCodecContext) {
		avcodec_close(m_pCodecContext);
		m_pCodecContext = NULL;
	}
	if (m_frame)
	{
		av_frame_unref(m_frame);
		av_frame_free(&m_frame);
	}

	m_frame = NULL;

	if (m_swr_ctx)
		swr_free(&m_swr_ctx);
	m_swr_ctx = NULL;
}

int FFMpegAudioEncoder::prepare_frame_buffers()
{
	int nb_samples;
	if (m_pCodecContext->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = m_pCodecContext->frame_size;

	m_frame = FFMpegOutputWapper::alloc_audio_frame(m_pCodecContext->sample_fmt, m_pCodecContext->channel_layout,
		m_pCodecContext->sample_rate, nb_samples);
	return 0;
}

int FFMpegAudioEncoder::prepare_resampler(int channels, int sample_rate, enum AVSampleFormat sample_fmt)
{
	int ret = 0;
	/* create resampler context */
	m_swr_ctx = swr_alloc();
	if (!m_swr_ctx) {
		fprintf(stderr, "Could not allocate resampler context\n");
		return -1;
	}

	/* set options */
	av_opt_set_int(m_swr_ctx, "in_channel_count", channels, 0);
	av_opt_set_int(m_swr_ctx, "in_sample_rate", sample_rate, 0);
	av_opt_set_sample_fmt(m_swr_ctx, "in_sample_fmt", sample_fmt, 0);
	av_opt_set_int(m_swr_ctx, "out_channel_count", m_pCodecContext->channels, 0);
	av_opt_set_int(m_swr_ctx, "out_sample_rate", m_pCodecContext->sample_rate, 0);
	av_opt_set_sample_fmt(m_swr_ctx, "out_sample_fmt", m_pCodecContext->sample_fmt, 0);

	/* initialize the resampling context */
	if ((ret = swr_init(m_swr_ctx)) < 0) {
		fprintf(stderr, "Failed to initialize the resampling context\n");
		return -1;
	}
	return ret;
}

AVFrame* FFMpegAudioEncoder::resampling(AVFrame* input)
{
	int64_t dst_nb_samples;
	/* convert samples from native format to destination codec format, using the resampler */
	/* compute destination number of samples */
	//== CODE ADD BY ANYZ =================================================================//
	// Date: 2016/04/05 17:12:55
	//m_pCodecContext->sample_rate = 44100;
	
	
	//== CODE END =========================================================================//
	dst_nb_samples = av_rescale_rnd(swr_get_delay(m_swr_ctx, m_pCodecContext->sample_rate) + input->nb_samples,
		m_pCodecContext->sample_rate, input->sample_rate, AV_ROUND_UP);
	_ASSERT(dst_nb_samples == input->nb_samples);

	/* when we pass a frame to the encoder, it may keep a reference to it
	* internally;
	* make sure we do not overwrite it here
	*/
	int ret = av_frame_make_writable(m_frame);
	if (ret < 0)
		return NULL;

	/* convert to destination format */
	ret = swr_convert(m_swr_ctx,
		m_frame->data, dst_nb_samples,
		(const uint8_t **)input->data, input->nb_samples);
	if (ret < 0) {
		fprintf(stderr, "Error while converting\n");
		return NULL;
	}
	m_frame->nb_samples = ret;
	m_frame->channels = m_pCodecContext->channels;
	m_frame->sample_rate = m_pCodecContext->sample_rate;
	m_frame->format = m_pCodecContext->sample_fmt;
	return m_frame;
}

AVPacket* FFMpegAudioEncoder::encode(AVFrame* inputframe)
{
	//Check if we need to do resampling
	if (pkt.buf)
		av_packet_unref(&pkt);
	//Every time, we need to clear the packet object
	memset(&pkt, 0, sizeof(AVPacket));
	av_init_packet(&pkt);

	if (m_pCodecContext->sample_rate != inputframe->sample_rate || m_pCodecContext->channels != inputframe->channels
		|| m_pCodecContext->sample_fmt != (AVSampleFormat)(inputframe->format))
	{
		if (m_swr_ctx == NULL)
			prepare_resampler(inputframe->channels, inputframe->sample_rate, (AVSampleFormat)(inputframe->format));
		inputframe = resampling(inputframe);
	}
	int got_packet = 0;


	//Add audio samples into a FIFO first
	if (av_audio_fifo_space(m_fifo) < inputframe->nb_samples)
		av_audio_fifo_realloc(m_fifo, av_audio_fifo_size(m_fifo) + inputframe->nb_samples);
	av_audio_fifo_write(m_fifo, (void **)inputframe->data, inputframe->nb_samples);

	if (av_audio_fifo_size(m_fifo) >= (m_pCodecContext->frame_size > 0 ? m_pCodecContext->frame_size : 1024))
	{
		AVFrame * frame = av_frame_alloc();
		frame->nb_samples = m_pCodecContext->frame_size > 0 ? m_pCodecContext->frame_size : 1024;
		frame->channel_layout = m_pCodecContext->channel_layout;
		frame->format = m_pCodecContext->sample_fmt;
		frame->sample_rate = m_pCodecContext->sample_rate;
		av_frame_get_buffer(frame, 0);
		av_audio_fifo_read(m_fifo, (void **)frame->data, (m_pCodecContext->frame_size > 0 ? m_pCodecContext->frame_size : 1024));

		//Update time stamp
		/** Set a timestamp based on the sample rate for the container. */
		if (frame) {
			frame->pts = samples_index;
			samples_index += frame->nb_samples;
		}
		//write the audio frame
		int ret = avcodec_encode_audio2(m_pCodecContext, &pkt, frame, &got_packet);
		if (ret < 0) {

			char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			fprintf(stderr, "Error encoding audio frame: %s\n", av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret));
			got_packet = 0;
		}

		av_frame_free(&frame);
	}

	if (got_packet)
	{	
		return &pkt;
	}		
	return NULL;
}

int FFMpegAudioEncoder::flushencoder(std::list<AVPacket*> & pktlist)
{
	int count = 0;
	while (av_audio_fifo_size(m_fifo) >= (m_pCodecContext->frame_size > 0 ? m_pCodecContext->frame_size : 1024))
	{
		AVPacket * thePkt = av_packet_alloc();		
		AVFrame * frame = av_frame_alloc();
		frame->nb_samples = m_pCodecContext->frame_size > 0 ? m_pCodecContext->frame_size : 1024;
		frame->channel_layout = m_pCodecContext->channel_layout;
		frame->format = m_pCodecContext->sample_fmt;
		frame->sample_rate = m_pCodecContext->sample_rate;
		av_frame_get_buffer(frame, 0);
		av_audio_fifo_read(m_fifo, (void **)frame->data, (m_pCodecContext->frame_size > 0 ? m_pCodecContext->frame_size : 1024));
		int got_packet = 0;
		//Update time stamp
		/** Set a timestamp based on the sample rate for the container. */
		if (frame) {
			frame->pts = samples_index;
			samples_index += frame->nb_samples;
		}
		//write the audio frame
		int ret = avcodec_encode_audio2(m_pCodecContext, thePkt, frame, &got_packet);
		if (ret < 0) {

			char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			fprintf(stderr, "Error encoding audio frame: %s\n", av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret));
			got_packet = 0;
		}

		av_frame_free(&frame);
		if (got_packet)
		{
			pktlist.push_back(thePkt);
			count++;
		}
	}
	return count;
}

FFMpegVideoEncoder::FFMpegVideoEncoder()
{
	memset(&pkt, 0, sizeof(pkt));
}


FFMpegVideoEncoder::~FFMpegVideoEncoder()
{
	if (m_pCodecContext) {
		avcodec_close(m_pCodecContext);
		m_pCodecContext = NULL;
	}
	if (m_frame)
	{
		av_frame_unref(m_frame);
		av_frame_free(&m_frame);
	}

	m_frame = NULL;

	if (m_sws_ctx)
		sws_freeContext(m_sws_ctx);
	m_sws_ctx = NULL;
}
int FFMpegVideoEncoder::prepare_frame_buffers()
{
	/* allocate and init a re-usable frame */
	m_frame = FFMpegOutputWapper::alloc_picture(m_pCodecContext->pix_fmt, m_pCodecContext->width, m_pCodecContext->height);
	if (!m_frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		return -1;
	}
	return 0;
}

bool FFMpegVideoEncoder::set_options(BSTR argv, int flags)
{
	_tstring argstrs[3] = { _T(""), _T(""), _T("") };
	SplitArgs(argv, argstrs);
	bool ret = (ParseOptions(&m_Options, argstrs[1], flags) >= 0);
	return ret;
}

AVFrame* FFMpegVideoEncoder::scale(AVFrame* input)
{

	int ret = sws_scale(m_sws_ctx, (const uint8_t* const*)input->data, input->linesize, 0, m_pCodecContext->height, m_frame->data, m_frame->linesize);
	if (ret < 0)
		return NULL;
	if (!m_frame->sample_aspect_ratio.num)    m_frame->sample_aspect_ratio = m_pCodecContext->sample_aspect_ratio;
	if (!m_frame->width)                      m_frame->width = m_pCodecContext->width;
	if (!m_frame->height)                     m_frame->height = m_pCodecContext->height;
	if (m_frame->format == AV_PIX_FMT_NONE)   m_frame->format = m_pCodecContext->pix_fmt;
	return m_frame;
}

//if we want to resample, we need to call this function before encoding
int FFMpegVideoEncoder::prepare_scaler(int srcW, int srcH, enum AVPixelFormat srcFormat)
{
	if (m_sws_ctx == NULL)
	{
		m_sws_ctx = sws_getContext(srcW, srcH, srcFormat, m_pCodecContext->width, m_pCodecContext->height, m_pCodecContext->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL); //AV_PIX_FMT_YUV420P
	}
	return 0;
}

AVPacket* FFMpegVideoEncoder::encode(AVFrame* inputframe)
{
	//Check if we need to do resampling
	if (pkt.buf)
		av_packet_unref(&pkt);
	//Every time, we need to clear the packet object
	memset(&pkt, 0, sizeof(AVPacket));
	av_init_packet(&pkt);

	m_pCodecContext->compression_level = 1;

	if (m_pCodecContext->width != inputframe->width || m_pCodecContext->height != inputframe->height
		|| m_pCodecContext->pix_fmt != (AVPixelFormat)(inputframe->format))
	{
		if (m_sws_ctx == NULL)
			prepare_scaler(inputframe->width, inputframe->height, (AVPixelFormat)(inputframe->format));
		inputframe = scale(inputframe);
	}
	int got_packet = 0;

	//Update time stamp
	/** Set a timestamp based on the sample rate for the container. */
	if (inputframe) {
		inputframe->pts = samples_index;
		samples_index ++;
	}

	int ret = avcodec_encode_video2(m_pCodecContext, &pkt, inputframe, &got_packet);
	if (ret < 0) {
		char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		fprintf(stderr, "Error encoding audio frame: %s\n", av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret));
		return NULL;
	}
	
	if (got_packet)
		return &pkt;
	return NULL;
}

int FFMpegVideoEncoder::flushencoder(std::list<AVPacket*> & pktlist)
{
	int count = 0;
	int got_packet = 0;
	do
	{
		AVPacket * thePkt = av_packet_alloc();
		//write the audio frame

		// add by anyz
		if (!thePkt->data || thePkt->size <= 0)
			return 0;
		int ret = avcodec_encode_video2(m_pCodecContext, thePkt, NULL, &got_packet);
		if (ret < 0) {
			char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			fprintf(stderr, "Error encoding audio frame: %s\n", av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret));
			got_packet = 0;
		}
		if (got_packet)
		{
			pktlist.push_back(thePkt);
			count++;
		}
	} while (got_packet);
	return count;
}