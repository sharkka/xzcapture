// XiaozaiWapper.h

#pragma once
#pragma warning(disable:4018)
#include <map>
extern "C" {
#include "libavformat\avformat.h"
#include "libavdevice\avdevice.h"
}
#include <objidl.h>

#ifndef SAFE_CLOSE_CONTEXT
#define SAFE_CLOSE_CONTEXT( X )					\
if (X != NULL)									\
{												\
	avformat_close_input(&X);					\
}
#endif

enum FFMpegInputType
{
	UNKNOWN = 0,
	AUDIO_ONLY,
	VIDEO_ONLY,
	AUDIO_AND_VIDEO
};

struct FFMpegVideoInfo
{
	int64_t bit_rate;
	AVRational time_base;
	int ticks_per_frame;
	int width, height;
	int gop_size;
	enum AVPixelFormat pix_fmt;
	/**
	* thread count
	* is used to decide how many independent tasks should be passed to execute()
	* - encoding: Set by user.
	* - decoding: Set by user.
	*/
	int thread_count;
	AVRational framerate;
};

struct FFMpegAudioInfo
{
	int64_t bit_rate;
	AVRational time_base;
	/* audio only */
	int sample_rate; ///< samples per second
	int channels;    ///< number of audio channels
	/**
	* audio sample format
	* - encoding: Set by user.
	* - decoding: Set by libavcodec.
	*/
	enum AVSampleFormat sample_fmt;  ///< sample format
	/**
	* Audio channel layout.
	* - encoding: set by user.
	* - decoding: set by user, may be overwritten by libavcodec.
	*/
	uint64_t channel_layout;

	
};

class FFMpegInputWapper
{
public:
	//the context of input format
	AVFormatContext *m_pFormatCtx = NULL;
private:
	//the option of the input format
	AVDictionary *m_fmtOptions = NULL;
	//the option of the input format
	AVDictionary *m_audioOptions = NULL;
	//the option of the input format
	AVDictionary *m_videoOptions = NULL;

	//the input filename or devicename
	const char* m_pFileName = NULL;

	AVPacket m_packet;

	FFMpegInputType m_InputType = UNKNOWN;
	
	bool m_isOpened = false;

	int m_VideoIndex = -1;
	int m_AudioIndex = -1;

protected:
	
	
	virtual void Cleanup(bool disposing);
	bool open_input(const char* filename, bool isDevice);
	//== CODE ADD BY ANYZ =================================================================//
	// Date: 2016/04/06 11:36:47
	void close_input() {
		if (m_fmtOptions) {
			av_dict_free(&m_fmtOptions);
			m_fmtOptions = NULL;
		}
		if (m_videoOptions && *((int*)(m_videoOptions))>0) {
			av_dict_free(&m_videoOptions);
			m_videoOptions = NULL;
		}
		m_audioOptions = NULL;
		/*if (m_audioOptions) {
			av_dict_free(&m_audioOptions);
			m_audioOptions = NULL;
		}*/
		av_packet_unref(&m_packet);
		AVPacket* p = (AVPacket*)&m_packet;
		av_packet_free(&p);
		SAFE_CLOSE_CONTEXT(m_pFormatCtx);
	}
	//== CODE END =========================================================================//
	
	// Get a frame
	AVPacket * get_frame_internal();

public:
	virtual ~FFMpegInputWapper(){
		Cleanup(true);
	};
	


	// Obtain the available DShow devices in the system
	//std::list<std::string> * get_input_device();

	// open a video/audio device
	bool open_input_device(const char* filename);
	void close_input_device();

	// open a video file
	bool open_input_file(const char* filename);

	///
	/// set the options of the input file/device, should be called before open
	///	 
	bool set_options(std::map<const char*, const char *> optionmap, int flags, int optindex = 0);
	bool set_options(BSTR argv, int flags);

	int GetStreamNum();

	FFMpegInputType GetInputType();
	bool IsOpened();

	
	double GetInputFrameRate();
	AVPacket* get_a_frame();
	bool set_output_callback();
	inline int get_videostream_index(){ return m_VideoIndex; }
	inline int get_audiostream_index(){ return m_AudioIndex; }

	inline AVCodecContext * get_video_stream_codec()
	{
		if (!IsOpened())
			return NULL;

		if (m_VideoIndex >= m_pFormatCtx->nb_streams)
			return NULL;
		return m_pFormatCtx->streams[m_VideoIndex]->codec;
	}

	inline AVCodecContext * get_audio_stream_codec()
	{
		if ((!IsOpened())||(m_AudioIndex==-1))
			return NULL;

		if (m_AudioIndex >= m_pFormatCtx->nb_streams)
			return NULL;
		return m_pFormatCtx->streams[m_AudioIndex]->codec;
	}

	//get the format of a stream
	AVCodecContext * get_stream_codec(unsigned int stream_index);
	//get video info
	int get_video_info(long &Width, long &Height, double &fps);

	int get_video_info(FFMpegVideoInfo & theInfo);

	int get_audio_info(FFMpegAudioInfo & theInfo);

	int get_audio_info(WORD& nChannels,         // Number of channels
		DWORD& nSamplesPerSec,   // Samples per second
		WORD& wBitsPerSample     // Bits per sample
		);
};
