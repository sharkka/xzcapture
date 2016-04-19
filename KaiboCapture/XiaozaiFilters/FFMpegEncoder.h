#pragma once
#include <list>
extern "C"
{
#include "libavutil/audio_fifo.h"
#include "libswresample/swresample.h"
}

class FFMpegEncoder
{
protected:
	AVCodecContext * m_pCodecContext = NULL;
	AVCodec *  m_pCodec = NULL;
	//the option of the codec
	AVDictionary *m_Options = NULL;
	bool m_isOpened = false;

	int64_t samples_index;
public:
	FFMpegEncoder();
	virtual ~FFMpegEncoder();
	virtual bool open_codec(AVCodecContext * codecContext);
	virtual bool open_codec(const char * codec_name);
	virtual bool open_codec(enum AVCodecID id);
	bool IsOpen(){ return m_isOpened; };
	///
	/// set the options of the codec, should be called before open
	///	 
	bool set_options(std::map<const char*, const char *> optionmap, int flags);

	virtual bool set_options(BSTR argv, int flags) = 0;

	AVCodecContext * get_codec_cxt(){ return m_pCodecContext; };
};

class FFMpegAudioEncoder :
	public FFMpegEncoder
{
	AVAudioFifo *m_fifo = NULL;   //this is used before audio encoding
	/* pts of the next frame that will be generated */
	

	AVFrame *m_frame = NULL;
	AVPacket pkt;    // data and size must be 0;

	struct SwrContext *m_swr_ctx = NULL;

	int prepare_frame_buffers();
	AVFrame* resampling(AVFrame* input);
	//if we want to resample, we need to call this function before encoding
	int prepare_resampler(int channels, int sample_rate, enum AVSampleFormat sample_fmt = AV_SAMPLE_FMT_S16);
public:
	FFMpegAudioEncoder();
	virtual ~FFMpegAudioEncoder();

	virtual bool open_codec(AVCodecContext * codecContext)
	{ 
		bool res = FFMpegEncoder::open_codec(codecContext); 
		m_fifo = av_audio_fifo_alloc(m_pCodecContext->sample_fmt, m_pCodecContext->channels, 4096);
		res = res && (prepare_frame_buffers()==0);
		return res;
	};
	virtual bool open_codec(const char * codec_name)
	{
		bool res = FFMpegEncoder::open_codec(codec_name);
		m_fifo = av_audio_fifo_alloc(m_pCodecContext->sample_fmt, m_pCodecContext->channels, 4096);
		res = res && (prepare_frame_buffers() == 0);
		return res;
	};
	virtual bool open_codec(enum AVCodecID id)
	{
		bool res = FFMpegEncoder::open_codec(id);
		m_fifo = av_audio_fifo_alloc(m_pCodecContext->sample_fmt, m_pCodecContext->channels, 4096);
		res = res && (prepare_frame_buffers() == 0);
		return res;
	};

	
	AVPacket* encode(AVFrame* inputframe);

	///
	/// the AVPackets in pktlist must be freed using av_packet_free().
	///
	int flushencoder(std::list<AVPacket*> & pktlist);

	bool set_options(BSTR argv, int flags);
	void setClosed() {
		m_isOpened = false;
	}
	//== CODE ADD BY ANYZ =================================================================//
	// Date: 2016/04/07 14:45:44
	void close_codec() {
		av_audio_fifo_free(m_fifo);

		if (m_frame) {
			av_frame_unref(m_frame);
		}

		if (m_swr_ctx) {
			swr_free(&m_swr_ctx);
			m_swr_ctx = NULL;
		}
		if (m_pCodecContext) {
			avcodec_close(m_pCodecContext);
			m_pCodecContext = NULL;
		}
		m_isOpened = false;
	}
	
	//== CODE END =========================================================================//
};

class FFMpegVideoEncoder :
	public FFMpegEncoder
{

	AVFrame *m_frame = NULL;
	AVPacket pkt;    // data and size must be 0;

	struct SwsContext *m_sws_ctx = NULL;

	int prepare_frame_buffers();
	AVFrame* scale(AVFrame* input);
	//if we want to resample, we need to call this function before encoding
	int prepare_scaler(int srcW, int srcH, enum AVPixelFormat srcFormat = AV_PIX_FMT_YUV420P);
public:
	FFMpegVideoEncoder();
	virtual ~FFMpegVideoEncoder();

	virtual bool open_codec(AVCodecContext * codecContext)
	{
		bool res = FFMpegEncoder::open_codec(codecContext);
		res = res && (prepare_frame_buffers() == 0);
		return res;
	};
	virtual bool open_codec(const char * codec_name)
	{
		bool res = FFMpegEncoder::open_codec(codec_name);
		res = res && (prepare_frame_buffers() == 0);
		return res;
	};
	virtual bool open_codec(enum AVCodecID id)
	{
		bool res = FFMpegEncoder::open_codec(id);
		res = res && (prepare_frame_buffers() == 0);
		return res;
	};

	AVPacket* encode(AVFrame* inputframe);

	///
	/// the AVPackets in pktlist must be freed using av_packet_free().
	///
	int flushencoder(std::list<AVPacket*> & pktlist);

	bool set_options(BSTR argv, int flags);

	void setClosed() {
		m_isOpened = false;
	}
	//== CODE ADD BY ANYZ =================================================================//
	// Date: 2016/04/07 14:46:04
	void close_codec() {
		if (m_pCodecContext) {
			avcodec_close(m_pCodecContext);
			m_pCodecContext = NULL;
		}
		if (m_frame) {
			av_frame_unref(m_frame);
		}

		if (m_sws_ctx) {
			sws_freeContext(m_sws_ctx);
			m_sws_ctx = NULL;
		}
		m_isOpened = false;
	}
	
	//== CODE END =========================================================================//
};