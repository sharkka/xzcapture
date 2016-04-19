#pragma once
#include "Win32_Utils.h"
extern "C" {
#include "libavformat\avformat.h"
#include "libavdevice\avdevice.h"
#include "libswscale\swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/avassert.h"

#include "libavutil/time.h"
}
#include <map>
#include <objidl.h>
#include "FFMpegInputWapper.h"

struct OutputStream {
	AVStream *st = NULL;

	/* pts of the next frame that will be generated */
	int64_t samples_count;

	AVFrame *frame = NULL;
	AVFrame *tmp_frame = NULL;

	struct SwsContext *sws_ctx = NULL;
	struct SwrContext *swr_ctx = NULL;
} ;

class FFMpegOutputWapper
{
	AVFormatContext *m_pFormatCtx = NULL;
	AVFormatContext *m_pRtmpFormatCtx = NULL;

	FFMpegVideoInfo m_VideoInfo;
	FFMpegAudioInfo m_AudioInfo;

	//the context of input format
	
	//the option of the input format
	AVDictionary *m_fmtOptions = NULL;
	//the option of the input format
	AVDictionary *m_audioOptions = NULL;
	//the option of the input format
	AVDictionary *m_videoOptions = NULL;


	//the output RTMP server url
	const char* m_pFileName = NULL;

	bool m_isOpened = false;

	//int m_VideoIndex = -1;
	//int m_AudioIndex = -1;

	//Currently, we only support two streams
	OutputStream m_VideoStream;
	OutputStream m_AudioStream;

	void add_stream(enum AVCodecID codec_id);
	void close_stream(OutputStream *ost);
	void open_video(const AVCodec *codec, AVDictionary *opt_arg);
	void open_audio(const AVCodec *codec, AVDictionary *opt_arg);
	int write_frame(const AVRational *time_base, AVStream *st, AVPacket *pkt);

public:
	void CleanUp(){
		
		close_stream(&m_VideoStream);
		close_stream(&m_AudioStream);
		if (m_pFormatCtx)
		{
			if (m_pFormatCtx->pb)
				avio_closep(&m_pFormatCtx->pb);
			avformat_free_context(m_pFormatCtx);
		}
		if (m_fmtOptions)
		{
			av_dict_free(&m_fmtOptions);
			m_fmtOptions = NULL;
		}
		if (m_videoOptions)
		{
			av_dict_free(&m_videoOptions);
			m_videoOptions = NULL;
		}
		if (m_audioOptions)
		{
			av_dict_free(&m_audioOptions);
			m_audioOptions = NULL;
		}
		m_pFormatCtx = NULL;
		m_isOpened = false;
	}


	FFMpegOutputWapper();
	virtual ~FFMpegOutputWapper();
	static AVFrame * alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
	static AVFrame * alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples);

	AVFormatContext * get_fmt_cxt() { return m_pFormatCtx; };
	inline AVCodecContext * get_video_stream_codec()
	{
		return m_VideoStream.st->codec;
	}

	inline AVCodecContext * get_audio_stream_codec()
	{
		return m_AudioStream.st->codec;
	}
	///
	///The follwing methods are used to create a brand new out file/rtmp push streaming
	///
	bool open_output2(const char * filename, bool bNeedEncoder = true);
	bool open_output(const char * filename, bool bNeedEncoder=false);
	bool IsOpened();

	void setClosed() {
		m_isOpened = false;
	}
	///
	/// set the options of the input file/device, should be called before open
	///	 
	bool set_options(std::map<const char*, const char *> optionmap, int flags, int optindex = 0);
	bool set_options(BSTR argv, int flags);
	
	///
	///set_video_info or set_audio_info should be called before open. 
	///Note: The values in these functions can be overwritten by set_options.
	///
	int set_video_info(FFMpegVideoInfo  theInfo){ m_VideoInfo = theInfo; return 0; };
	int set_audio_info(FFMpegAudioInfo  theInfo){ m_AudioInfo = theInfo; return 0; };
		
	//Write the raw video data to be encoded, and write to the file
	//Note: only valid when bNeedEncoder==TRUE
	int write_video_frame(AVFrame *frame);

	//Write the raw audio data to be encoded, and write to the file
	//Note: only valid when bNeedEncoder==TRUE
	int write_audio_frame(AVFrame *frame);

	//Write compressed packet, user should make sure the formate of pkt matchs the destinated file
	int write_video_frame(AVPacket *pkt)
	{
		return write_frame(&m_VideoStream.st->codec->time_base, m_VideoStream.st, pkt);
	};

	//Write the compressed audio data to be encoded, and write to the file
	//the user should make sure the formate of pkt matchs the destinated file
	int write_audio_frame(AVPacket *pkt)
	{
		return write_frame(&m_AudioStream.st->codec->time_base, m_AudioStream.st, pkt);		
	};

	///
	///the following methods are used to copy an input file to an out file
	///
	bool open_output(const char * filename, AVFormatContext *pAVInputCtx);

	int do_copy_loop(AVFormatContext * pAVInFmtCtx, bool isStreaming = false);

	//used by both modes
	int flush()
	{
		if (m_isOpened)
		{
			av_interleaved_write_frame(m_pFormatCtx, NULL); 
			if (m_pFormatCtx->pb)
			avio_flush(m_pFormatCtx->pb);
			av_write_trailer(m_pFormatCtx); 
			if (m_pFormatCtx->pb)
			avio_closep(&m_pFormatCtx->pb);
		}
		else
			return -1;
		return 0;
	};
};

