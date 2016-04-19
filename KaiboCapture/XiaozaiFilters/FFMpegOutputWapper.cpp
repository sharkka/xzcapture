#include "stdafx.h"
#include "Config.h"
#include "FFMpegOutputWapper.h"
#pragma warning(disable:4010 4244)

// a wrapper around a single output AVStream
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC

int SplitArgs(BSTR argv, _tstring argstrs[3]);
int ParseOptions(AVDictionary **options, _tstring argv, int flags);

extern void logFF(int level, const char *fmt, ...);

FFMpegOutputWapper::FFMpegOutputWapper()
{
	m_AudioInfo.sample_fmt = AV_SAMPLE_FMT_FLTP;
	//m_AudioInfo.sample_fmt = AV_SAMPLE_FMT_S16;
	m_AudioInfo.bit_rate = 64000;
	m_AudioInfo.channels = 2;
	m_AudioInfo.sample_rate = 44100;
	m_AudioInfo.channel_layout = AV_CH_LAYOUT_STEREO;
	m_AudioInfo.time_base = AVRational{ 1, m_AudioInfo.sample_rate};

	m_VideoInfo.bit_rate = 1000000;
	m_VideoInfo.gop_size = 90;
	m_VideoInfo.pix_fmt = STREAM_PIX_FMT;
	m_VideoInfo.thread_count = 8;
	//m_VideoInfo.ticks_per_frame = 2;
	m_VideoInfo.time_base = AVRational{ 1, STREAM_FRAME_RATE};

	m_VideoInfo.width = 640;
	m_VideoInfo.height = 480;
}


FFMpegOutputWapper::~FFMpegOutputWapper()
{
	CleanUp();
}


bool FFMpegOutputWapper::open_output(const char * filename, bool bNeedEncoder)
{

	m_VideoInfo.bit_rate = 512000;

	if (m_isOpened)
		return false;

	if (m_pFormatCtx != NULL)
	{												
		avformat_free_context(m_pFormatCtx);
		m_pFormatCtx = NULL;
	}

	//push a stream to a RTMP server
	//f4v == AAC + H.264
	//int ret = avformat_alloc_output_context2(&m_pFormatCtx, NULL, "f4v", filename);


#if 1
	int ret = 0;
	if (!memcmp(filename, "rtmp", 4)) {
		ret = avformat_alloc_output_context2(&m_pFormatCtx, NULL, "flv", filename);
		m_pFormatCtx->oformat->video_codec = AV_CODEC_ID_H264;
		m_pFormatCtx->oformat->audio_codec = AV_CODEC_ID_AAC;

	} else {
		ret = avformat_alloc_output_context2(&m_pFormatCtx, NULL, "f4v", filename);
	}
#endif

	if ((ret < 0) || (m_pFormatCtx == NULL))
	{
		//TODO: print messages for errors
		return false;
	}

	//Add video stream
	add_stream(m_pFormatCtx->oformat->video_codec);
	//Add audio stream
	add_stream(m_pFormatCtx->oformat->audio_codec);

	//Open codec
	if (bNeedEncoder)
	{
		open_video(m_VideoStream.st->codec->codec, m_videoOptions);
		open_audio(m_AudioStream.st->codec->codec, m_audioOptions);
	}

	if (!(m_pFormatCtx->oformat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&m_pFormatCtx->pb, filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "Could not open output file '%s'", filename);
			CleanUp();
			return false;
		}
	}

	/* Write the stream header, if any. */
	ret = avformat_write_header(m_pFormatCtx, &m_fmtOptions);
	if (m_pFormatCtx->streams[0]->codec->coded_width > 0)
	printf("GGGGGGGGGGGGG: %d x %d\n", m_pFormatCtx->streams[0]->codec->coded_width,
		m_pFormatCtx->streams[0]->codec->coded_height);
	if (ret < 0) {
		CleanUp();
		return false;
	}

	m_isOpened = true;
	return true;
	
}


bool FFMpegOutputWapper::IsOpened()
{
	return (m_isOpened && (m_pFormatCtx != NULL));
}


bool FFMpegOutputWapper::set_options(std::map<const char*, const char *> optionmap, int flags, int optindex)
{
	AVDictionary ** theoption;
	switch (optindex)
	{
	case 0:
		theoption = &m_fmtOptions;
		break;
	case 1:
		theoption = &m_videoOptions;
		break;
	case 2:
		theoption = &m_audioOptions;
		break;
	default:
		return false;
	}
	int ret;
	for each (std::pair<const char*, const char *> item in optionmap)
	{
		ret = av_dict_set(theoption, item.first, item.second, flags);
		if (ret < 0)
			return false;
	}
	return true;
}


/*bool FFMpegOutputWapper::set_options(BSTR argv, int flags)
{
	if (argv == NULL)
		return false;
	_tstring argstrs[3];
	SplitArgs(argv, argstrs);

	bool ret = (ParseOptions(&m_fmtOptions, argstrs[0], flags) >= 0);
	ret = (ParseOptions(&m_videoOptions, argstrs[1], flags) >= 0);
	ret = (ParseOptions(&m_audioOptions, argstrs[2], flags) >= 0);
	return ret;
}*/

bool FFMpegOutputWapper::set_options(BSTR argv, int flags)
{
	if (argv == NULL)
		return false;
	_tstring argstrs[3];
	SplitArgs(argv, argstrs);

	bool ret;
	ret = (ParseOptions(&m_fmtOptions, argstrs[0], flags) >= 0);
	ret = (ParseOptions(&m_videoOptions, argstrs[1], flags) >= 0);
	ret = (ParseOptions(&m_audioOptions, argstrs[2], flags) >= 0);

	return ret;
}

/* Add an output stream. */
void FFMpegOutputWapper::add_stream(enum AVCodecID codec_id)
{
	AVCodecContext *c;
	int i;

	/* find the encoder */
	AVCodec *codec = avcodec_find_encoder(codec_id);
	if (!(codec)) {
		logFF(XZ_LOG_ERROR, "Could not find encoder for '%s'\n",
			avcodec_get_name(codec_id));
		exit(1);
	}

	AVStream* st = avformat_new_stream(m_pFormatCtx, codec);
	if (!st) {
		fprintf(stderr, "Could not allocate stream\n");
		exit(1);
	}
	st->id = m_pFormatCtx->nb_streams - 1;
	c = st->codec;
	
	switch ((codec)->type) {
	case AVMEDIA_TYPE_AUDIO:
		m_AudioStream.st = st;
		c->sample_fmt = (codec)->sample_fmts ?
			(codec)->sample_fmts[0] : m_AudioInfo.sample_fmt;
		c->bit_rate = m_AudioInfo.bit_rate;
		c->sample_rate = m_AudioInfo.sample_rate;
		if ((codec)->supported_samplerates) {
			c->sample_rate = (codec)->supported_samplerates[0];
			for (i = 0; (codec)->supported_samplerates[i]; i++) {
				if ((codec)->supported_samplerates[i] == m_AudioInfo.sample_rate)
					c->sample_rate = m_AudioInfo.sample_rate;
			}
		}
		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
		c->channel_layout = m_AudioInfo.channel_layout;
		if ((codec)->channel_layouts) {
			c->channel_layout = (codec)->channel_layouts[0];
			for (i = 0; (codec)->channel_layouts[i]; i++) {
				if ((codec)->channel_layouts[i] == m_AudioInfo.channel_layout)
					c->channel_layout = m_AudioInfo.channel_layout;
			}
		}
		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
		st->time_base = m_AudioInfo.time_base;
		c->time_base = st->time_base;
		break;

	case AVMEDIA_TYPE_VIDEO:
		m_VideoStream.st = st;
		c->codec_id = codec_id;

		c->bit_rate = m_VideoInfo.bit_rate;
		/* Resolution must be a multiple of two. */
		c->width = m_VideoInfo.width;
		c->height = m_VideoInfo.height;
		/* timebase: This is the fundamental unit of time (in seconds) in terms
		* of which frame timestamps are represented. For fixed-fps content,
		* timebase should be 1/framerate and timestamp increments should be
		* identical to 1. */
		st->time_base = m_VideoInfo.time_base;
		c->time_base = st->time_base;
		c->thread_count = m_VideoInfo.thread_count;
		c->framerate = m_VideoInfo.framerate;
		

		c->gop_size = m_VideoInfo.gop_size; /* emit one intra frame every twelve frames at most */
		c->pix_fmt = m_VideoInfo.pix_fmt;
		if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
			/* just for testing, we also add B frames */
			c->max_b_frames = 2;
		}
		if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
			/* Needed to avoid using macroblocks in which some coeffs overflow.
			* This does not happen with normal video, it just happens here as
			* the motion of the chroma plane does not match the luma plane. */
			c->mb_decision = 2;
		}
		break;

	default:
		break;
	}

	/* Some formats want stream headers to be separate. */
	if (m_pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

void FFMpegOutputWapper::close_stream(OutputStream *ost)
{
	if (ost->st != NULL && ost->st->codec!=NULL)
		avcodec_close(ost->st->codec);
	if (ost->frame)
		av_frame_free(&ost->frame);
	ost->frame = NULL;
	if (ost->tmp_frame)
		av_frame_free(&ost->tmp_frame);
	ost->tmp_frame = NULL;
	if (ost->sws_ctx)
		sws_freeContext(ost->sws_ctx);
	ost->sws_ctx = NULL;
	if (ost->swr_ctx)
		swr_free(&ost->swr_ctx);
	ost->swr_ctx = NULL;
}

AVFrame * FFMpegOutputWapper::alloc_picture( AVPixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	int ret;

	picture = av_frame_alloc();
	if (!picture)
		return NULL;

	picture->format = pix_fmt;
	picture->width = width;
	picture->height = height;

	/* allocate the buffers for the frame data */
	ret = av_frame_get_buffer(picture, 32);
	if (ret < 0) {
		logFF(XZ_LOG_ERROR, "Could not allocate frame data.\n");
		exit(1);
	}

	return picture;
}


void FFMpegOutputWapper::open_video(const AVCodec *codec, AVDictionary *opt_arg)
{
	int ret;
	AVCodecContext *c = m_VideoStream.st->codec;
	AVDictionary *opt = NULL;

	av_dict_copy(&opt, opt_arg, 0);

	// open the codec
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		logFF(XZ_LOG_ERROR, "Could not open video codec: %s\n", av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret));
		return;
	}

	/* allocate and init a re-usable frame */
	m_VideoStream.frame = alloc_picture(c->pix_fmt, c->width, c->height);
	if (!m_VideoStream.frame) {
		logFF(XZ_LOG_ERROR, "Could not allocate video frame\n");
		return;
	}

	/* If the output format is not YUV420P, then a temporary YUV420P
	* picture is needed too. It is then converted to the required
	* output format. */
	m_VideoStream.tmp_frame = NULL;
	if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
		m_VideoStream.tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
		if (!m_VideoStream.tmp_frame) {
			logFF(XZ_LOG_ERROR, "Could not allocate temporary picture\n");
			return;
		}
	}
}

void FFMpegOutputWapper::open_audio(const AVCodec *codec, AVDictionary *opt_arg)
{
	AVCodecContext *c;
	int nb_samples;
	int ret;
	AVDictionary *opt = NULL;

	c = m_AudioStream.st->codec;

	/* open it */
	av_dict_copy(&opt, opt_arg, 0);
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		printf("avcodec_open2 open failed., return %d\n", ret);
		logFF(XZ_LOG_ERROR, "Could not open audio codec: %s\n", av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret));
		return;
	}

	if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = c->frame_size;

	m_AudioStream.frame = alloc_audio_frame(c->sample_fmt, c->channel_layout,
		c->sample_rate, nb_samples);
	m_AudioStream.tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
		c->sample_rate, nb_samples);

	/* create resampler context */
	m_AudioStream.swr_ctx = swr_alloc();
	if (!m_AudioStream.swr_ctx) {
		logFF(XZ_LOG_ERROR, "Could not allocate resampler context\n");
		return;
	}

	/* set options */
	av_opt_set_int(m_AudioStream.swr_ctx, "in_channel_count", c->channels, 0);
	av_opt_set_int(m_AudioStream.swr_ctx, "in_sample_rate", c->sample_rate, 0);
	av_opt_set_sample_fmt(m_AudioStream.swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	av_opt_set_int(m_AudioStream.swr_ctx, "out_channel_count", c->channels, 0);
	av_opt_set_int(m_AudioStream.swr_ctx, "out_sample_rate", c->sample_rate, 0);
	av_opt_set_sample_fmt(m_AudioStream.swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

	/* initialize the resampling context */
	if ((ret = swr_init(m_AudioStream.swr_ctx)) < 0) {
		logFF(XZ_LOG_ERROR, "Failed to initialize the resampling context\n");
		return;
	}
}

/**************************************************************/
/* audio output */

AVFrame * FFMpegOutputWapper::alloc_audio_frame(enum AVSampleFormat sample_fmt,
	uint64_t channel_layout, int sample_rate, int nb_samples)
{
	AVFrame *frame = av_frame_alloc();
	int ret;

	if (!frame) {
		logFF(XZ_LOG_ERROR, "Error allocating an audio frame\n");
		return NULL;
	}

	frame->format = sample_fmt;
	frame->channel_layout = channel_layout;
	frame->sample_rate = sample_rate;
	frame->nb_samples = nb_samples;

	if (nb_samples) {
		ret = av_frame_get_buffer(frame, 0);
		if (ret < 0) {
			logFF(XZ_LOG_ERROR, "Error allocating an audio buffer\n");
			return NULL;
		}
	}
	return frame;
}

/*
* encode one video frame and send it to the muxer
* return 1 when encoding is finished, 0 otherwise
*/
int FFMpegOutputWapper::write_video_frame(AVFrame *frame)
{
	int ret;
	AVCodecContext *c;
	int got_packet = 0;
	AVPacket pkt = { 0 };

	c = m_VideoStream.st->codec;

	//frame = get_video_frame(ost);

	av_init_packet(&pkt);

	/* encode the image */
	ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
	if (ret < 0) {
		char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		//printf("Error encoding video frame : %s\n", av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret));
		logFF(XZ_LOG_ERROR, "Error encoding video frame: %s\n", av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret));
		return -1;
	}

	if (got_packet) {
		ret = write_frame(&c->time_base, m_VideoStream.st, &pkt);
		//av_packet_unref(&pkt);
		//av_packet_free(&pkt);
		
	}
	else {
		ret = 0;
	}

	if (ret < 0) {
		char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		logFF(XZ_LOG_ERROR, "Error while writing video frame: %s\n", av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret));
		return -1;
	}

	return (frame || got_packet) ? 0 : 1;
}

static int frame_index = 0;
int FFMpegOutputWapper::write_frame(const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
	//if (pkt->pts != pkt->dts)
	//	printf("%s pts: %I64d, dts: %I64d, duration: %I64d\n", \
		st == m_AudioStream.st ? "AUDIO" : "VIDEO", \
		pkt->pts, \
		pkt->dts, \
		pkt->duration);
	// rescale output packet timestamp values from codec to stream timebase
	av_packet_rescale_ts(pkt, *time_base, st->time_base);
	//av_packet_rescale_ts(pkt, *time_base, AVRational{ 1, STREAM_FRAME_RATE });
	

	pkt->stream_index = st->index;

	//pkt->pts = 0;
	//pkt->dts = 0;
	// Write the compressed frame to the media file
	if (st == m_AudioStream.st)
	{
		logFF(XZ_LOG_VERBOSE, "Add an audio frame pts=%I64d, dts=%I64d, size=%d, timebase=%d:%d \n", pkt->pts, pkt->dts, pkt->size, st->time_base.num, st->time_base.den);
	}
	else
	{
		logFF(XZ_LOG_VERBOSE, "Add an video frame pts=%I64d, dts=%I64d, size=%d, timebase=%d:%d  \n", pkt->pts, pkt->dts, pkt->size, st->time_base.num, st->time_base.den);
	}
	int pktlen = pkt->size;
	
	int ret = av_interleaved_write_frame(m_pFormatCtx, pkt);

	if (ret < 0) {
		logFF(XZ_LOG_VERBOSE, "av_interleaved_write_frame failed, frame index: %d\n", frame_index);
		printf("av_interleaved_write_frame failed.\n");
	}
	if (pkt->stream_index == 0) {
		if (!memcmp(m_pFormatCtx->filename, "rtmp", 4)) {
			//printf("%d - Send %d bytes frames, pts: %I64d, dts: %I64d, duration: %I64d, av_interleaved_write_frame returned %d\n", \
				frame_index, pktlen, \
				pkt->pts, \
				pkt->dts, \
				pkt->duration, \
				ret);

			frame_index++;
		}
	}
	if (pkt) {
		av_packet_unref(pkt);
		//av_packet_free(&pkt);
	}

	return ret;
}

/*
* encode one audio frame and send it to the muxer
* return 1 when encoding is finished, 0 otherwise, -1 if error
*/
int FFMpegOutputWapper::write_audio_frame(AVFrame *frame)
{
	AVCodecContext *c;
	AVPacket pkt = { 0 }; // data and size must be 0;
	int ret;
	int got_packet = 0;
	int64_t dst_nb_samples;

	av_init_packet(&pkt);
	c = m_AudioStream.st->codec;

	//frame = get_audio_frame(ost);

	if (frame) {
		/* convert samples from native format to destination codec format, using the resampler */
		/* compute destination number of samples */
		dst_nb_samples = av_rescale_rnd(swr_get_delay(m_AudioStream.swr_ctx, c->sample_rate) + frame->nb_samples,
			c->sample_rate, c->sample_rate, AV_ROUND_UP);
		av_assert0(dst_nb_samples == frame->nb_samples);

		/* when we pass a frame to the encoder, it may keep a reference to it
		* internally;
		* make sure we do not overwrite it here
		*/
		ret = av_frame_make_writable(m_AudioStream.frame);
		if (ret < 0)
			return (-1);

		/* convert to destination format */
		ret = swr_convert(m_AudioStream.swr_ctx,
			m_AudioStream.frame->data, dst_nb_samples,
			(const uint8_t **)frame->data, frame->nb_samples);
		if (ret < 0) {
			fprintf(stderr, "Error while converting\n");
			return (-1);
		}
		frame = m_AudioStream.frame;

		frame->pts = av_rescale_q(m_AudioStream.samples_count, AVRational{ 1, c->sample_rate }, c->time_base);
		m_AudioStream.samples_count += dst_nb_samples;
	}

	do{
		ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
		if (ret < 0) {
			char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			fprintf(stderr, "Error encoding audio frame: %s\n", av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret));
			break;
		}

		if (got_packet) {
			ret = write_frame(&c->time_base, m_AudioStream.st, &pkt);
			if (ret < 0) {
				char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
				fprintf(stderr, "Error while writing audio frame: %s\n",
					av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret));
				break;
			}
		}
	} while (0);
	av_packet_unref(&pkt);
	if (ret < 0) 
		return ret;
	return (frame || got_packet) ? 0 : 1;
}


bool FFMpegOutputWapper::open_output(const char * filename, AVFormatContext *pAVInputCtx)
{
	if (m_isOpened)
		return false;

	if (m_pFormatCtx != NULL)
	{
		avformat_free_context(m_pFormatCtx);
		m_pFormatCtx = NULL;
	}

	//push a stream to a RTMP server
	//f4v == AAC + H.264
	//mp4 == AAC + AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG4
	int ret = avformat_alloc_output_context2(&m_pFormatCtx, NULL, "flv", filename);
	if ((ret < 0) || (m_pFormatCtx == NULL))
	{
		//TODO: print messages for errors
		return false;
	}

	AVOutputFormat *ofmt = NULL;

	ofmt = m_pFormatCtx->oformat;

	for (unsigned int i = 0; i < pAVInputCtx->nb_streams; i++) {
		AVStream *in_stream = pAVInputCtx->streams[i];
		AVStream *out_stream = avformat_new_stream(m_pFormatCtx, in_stream->codec->codec);
		if (!out_stream) {
			fprintf(stderr, "Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			CleanUp();
			return false;
		}
		switch (in_stream->codec->codec_type) {
		case AVMEDIA_TYPE_AUDIO:
			m_AudioStream.st = out_stream;
			break;
		case AVMEDIA_TYPE_VIDEO:
			m_VideoStream.st = out_stream;
			break;
		default:
			break;
		}
		ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
		if (ret < 0) {
			fprintf(stderr, "Failed to copy context from input to output stream codec context\n");
			CleanUp();
			return false;
		}
		out_stream->codec->codec_tag = 0;
		if (m_pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	if (!(m_pFormatCtx->oformat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&m_pFormatCtx->pb, filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "Could not open output file '%s'", filename);
			CleanUp();
			return false;
		}
	}

	//av_bitstream_filter_filter
	/* Write the stream header, if any. */
	ret = avformat_write_header(m_pFormatCtx, &m_fmtOptions);
	if (ret < 0) {
		CleanUp();
		return false;
	}

	m_isOpened = true;
	return true;
}

int FFMpegOutputWapper::do_copy_loop(AVFormatContext * pAVInFmtCtx, bool isStreaming)
{
	AVPacket pkt;
	int ret = 0;
	while (1) {
		//TODO: In streaming mode, we need to wait a while
		AVStream *in_stream, *out_stream;
		ret = av_read_frame(pAVInFmtCtx, &pkt);
		if (ret < 0)
			break;

		in_stream = pAVInFmtCtx->streams[pkt.stream_index];
		out_stream = m_pFormatCtx->streams[pkt.stream_index];

		//log_packet(ifmt_ctx, &pkt, "in");

		/* copy packet */
		//|AV_ROUND_PASS_MINMAX
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF);

		//| AV_ROUND_PASS_MINMAX
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF );
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		//log_packet(ofmt_ctx, &pkt, "out");

		ret = av_interleaved_write_frame(m_pFormatCtx, &pkt);		
		if (ret < 0) {
			fprintf(stderr, "Error muxing packet\n");
			break;
		}
		av_packet_unref(&pkt);
	}
	return ret;
}