// This is the main DLL file.

#include "stdafx.h"
#include "list"
#include "string"
#include "Win32_Utils.h"
#include <vector>
#include "FFMpegInputWapper.h"
#include <gdiplus.h>

using namespace Gdiplus;

int SplitArgs(BSTR argv, _tstring argstrs[3])
{
	if (argv == NULL)
		return -1;
	std::vector<_tstring> strs;
	OS::SplitString(OS::StringW2T(argv), _T('$'), strs);
	int ret = 0;
	if (strs.size() > 0)
	{

		std::vector<_tstring> apair;
		for each (_tstring a in strs)
		{			
			OS::SplitString(a, _T(':'), apair);
			if (apair.size() != 2)
				continue;
			//ret = av_dict_set(options, OS::StringT2A(apair[0]).c_str(), OS::StringT2A(apair[1]).c_str(), flags);
			if (apair[0] == _T("-fmt"))
			{
				argstrs[0] = apair[1];
				ret++;
			}
			else if (apair[0] == _T("-codecv"))
			{
				argstrs[1] = apair[1];
				ret++;
			}
			else if (apair[0] == _T("-codeca"))
			{
				argstrs[2] = apair[1];
				ret++;
			}
			else
			{
			}			
		}
		apair.clear();
	}

	return ret;
}

int ParseOptions(AVDictionary **options, _tstring argv, int flags)
{
	if (argv.length() <1)
		return -1;
	int ret = av_dict_parse_string(options, OS::StringT2A(argv).c_str(), "=", ";", flags);
	return 0;
	
}

/******************************************************************************************************************************
* Function    : open_input_device
* File Path   : D:\work\stage\xz\xzcapture\XiaozaiCapture0301\XiaozaiFilters
* Brief       : 更改原来的接口，支持输入文件、设备、rtmp流地址
* Parameter   : @
* Return      : 
* Key Words   :
* Purpose     : 
* Author      : ANYZ
* Create Date : 2016/03/23 14:38:58
* Modify Date : 2016/03/23 14:38:58
******************************************************************************************************************************/
bool FFMpegInputWapper::open_input_device(const char* devicename)
{
	if (!memcmp(devicename, "rtmp://", 7)) {
		return open_input(devicename, false);
	} 
	//else if (NULL == devicename) {
	//	return open_input(devicename, true);
	//} 
	else {
		return open_input(devicename, true);
	}
}


	/*bool FFMpegInputWapper::open_input_device(const char* devicename)
	{
		return open_input(devicename,true);
	}*/

	void FFMpegInputWapper::close_input_device()
	{
		close_input();
	}

	bool FFMpegInputWapper::open_input(const char* filename, bool isDevice)
	{
		SAFE_CLOSE_CONTEXT(m_pFormatCtx);
		AVInputFormat *ifmt = NULL;

		if (isDevice)
			ifmt = av_find_input_format("dshow");
		else
			ifmt = av_find_input_format(filename);
		do {
			//== CODE ADD BY ANYZ =================================================================//
			// Date: 2016/04/06 11:50:30
			if (!memcmp(filename, "rtmp:", strlen("rtmp:"))) {
			//	av_dict_set(&m_videoOptions, "timeout", "6", 0); // in secs
			}
			//== CODE END =========================================================================//

			int res = avformat_open_input(&m_pFormatCtx, filename, ifmt, &m_fmtOptions);

			if (res != 0)
			{
				char errstr[100];
				av_strerror(res, errstr, 100);
				break;
			}

			AVDictionary** myOptions = new AVDictionary*[m_pFormatCtx->nb_streams];			
			//find the video/AUDIO streamS
			m_VideoIndex = -1;
			for (unsigned int i = 0; i < m_pFormatCtx->nb_streams; i++)
			{
				myOptions[i] = NULL;
				if (m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
				{
					m_VideoIndex = i;
					myOptions[i] = m_videoOptions;

				}
				else if (m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
				{
					myOptions[i] = m_audioOptions;
					m_AudioIndex = i;
				}
			}

			//if no video stream
			if (m_VideoIndex == -1)
			{
				if (m_AudioIndex == -1)
				{
					m_InputType = UNKNOWN;
					// memory leak fix
					delete[] myOptions;
					myOptions = NULL;
					break;
				}
				else
					m_InputType = AUDIO_ONLY;
			}
			else
			{
				if (m_AudioIndex == -1)
					m_InputType = VIDEO_ONLY;
				else
					m_InputType = AUDIO_AND_VIDEO;
			}

			if (myOptions[0] != NULL || myOptions[1] != NULL)
				res = avformat_find_stream_info(m_pFormatCtx, myOptions);
			delete[] myOptions;
			myOptions = NULL;
			//res = avformat_find_stream_info(m_pFormatCtx, &m_Options);
			if (res < 0)
				break;

			m_isOpened = true;
			m_pFileName = filename;
			return true;

		} while (false);

		SAFE_CLOSE_CONTEXT(m_pFormatCtx);
		return false;
	}

	void FFMpegInputWapper::Cleanup(bool disposing)
	{ 		
		/*if (m_fmtOptions)
		{
			av_dict_free(&m_fmtOptions);
			m_fmtOptions = NULL;
		}
		if (m_videoOptions && *((int*)(m_videoOptions))>0)
		{
			av_dict_free(&m_videoOptions);
			m_videoOptions = NULL;

		}
		if (m_audioOptions)
		{
			av_dict_free(&m_audioOptions);
			m_audioOptions = NULL;
		}*/
		av_packet_unref(&m_packet);
		SAFE_CLOSE_CONTEXT(m_pFormatCtx);
	}

	bool FFMpegInputWapper::open_input_file(const char* filename)
	{
		return open_input(filename, false);
	}


	bool FFMpegInputWapper::set_options(std::map<const char*, const char *> optionmap, int flags, int optindex)
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

	bool FFMpegInputWapper::set_options(BSTR argv, int flags)
	{
		if (argv == NULL)
			return false;
		_tstring argstrs[3];
		SplitArgs(argv, argstrs);

		bool ret= (ParseOptions(&m_fmtOptions, argstrs[0], flags)>=0);
		ret =  (ParseOptions(&m_videoOptions, argstrs[1], flags) >= 0);
		ret =  (ParseOptions(&m_audioOptions, argstrs[2], flags) >= 0);
		return ret;
	}

	int FFMpegInputWapper::GetStreamNum()
	{
		if (m_isOpened) 
			return m_pFormatCtx->nb_streams;
		else
			return -1;
	}


	FFMpegInputType FFMpegInputWapper::GetInputType()
	{
		return m_InputType;
	}


	bool FFMpegInputWapper::IsOpened()
	{
		return (m_isOpened)&&(m_pFormatCtx != NULL);
	}


	// Get a frame
	// Users do not need to free the packet as it will be managed by the wapper
	AVPacket * FFMpegInputWapper::get_frame_internal()
	{
		if (!IsOpened())
			return NULL;
		
		int ret = av_read_frame(m_pFormatCtx, &m_packet);
		if (ret >= 0)
			return &m_packet;

		return NULL;
	}


	double FFMpegInputWapper::GetInputFrameRate()
	{
		if (!IsOpened())
			return -1;
		//if no video stream
		if (m_VideoIndex == -1)
			return -1;
		AVRational myrate = m_pFormatCtx->streams[m_VideoIndex]->avg_frame_rate;
		if (myrate.den == 0)
			return -1;
		return double(myrate.num) / double(myrate.den);
		
	}


	AVPacket* FFMpegInputWapper::get_a_frame()
	{
		return get_frame_internal();
	}


	bool FFMpegInputWapper::set_output_callback()
	{
		return false;
	}


	AVCodecContext * FFMpegInputWapper::get_stream_codec(unsigned int stream_index)
	{
		if (!IsOpened())
			return NULL;

		if (stream_index >= m_pFormatCtx->nb_streams)
			return NULL;
		return m_pFormatCtx->streams[stream_index]->codec;
	}

	int FFMpegInputWapper::get_video_info(long &Width, long &Height, double &fps)
	{
		if (!m_isOpened)
			return -1;
		Width = m_pFormatCtx->streams[m_VideoIndex]->codec->width;
		Height = m_pFormatCtx->streams[m_VideoIndex]->codec->height;
		if (m_pFormatCtx->streams[m_VideoIndex]->codec->framerate.den)
			fps = ((double)m_pFormatCtx->streams[m_VideoIndex]->r_frame_rate.num) / ((double)m_pFormatCtx->streams[m_VideoIndex]->r_frame_rate.den);
		else
			fps = 0;
		return 0;
	}

	int FFMpegInputWapper::get_audio_info(FFMpegAudioInfo& theInfo)
	{
		if (!m_isOpened)
			return -1;
		AVCodecContext*pcxt= m_pFormatCtx->streams[m_AudioIndex]->codec;
		theInfo.bit_rate = pcxt->bit_rate;
		theInfo.channels = pcxt->channels;
		theInfo.channel_layout = pcxt->channel_layout;
		theInfo.sample_fmt = pcxt->sample_fmt;
		theInfo.sample_rate = pcxt->sample_rate;
		theInfo.time_base = pcxt->time_base;
		return 0;
	}

	int FFMpegInputWapper::get_video_info(FFMpegVideoInfo & theInfo)
	{
		if (!m_isOpened)
			return -1;
		AVCodecContext*pcxt = m_pFormatCtx->streams[m_VideoIndex]->codec;
		theInfo.bit_rate = pcxt->bit_rate;
		theInfo.gop_size = pcxt->gop_size;
		theInfo.height = pcxt->height;
		theInfo.pix_fmt = pcxt->pix_fmt;
		theInfo.thread_count = pcxt->thread_count;
		theInfo.ticks_per_frame = pcxt->ticks_per_frame;
		theInfo.time_base = pcxt->time_base;
		theInfo.width = pcxt->width;
		theInfo.framerate = pcxt->framerate;
		return 0;
	}

	int FFMpegInputWapper::get_audio_info(WORD& nChannels,         // Number of channels
		DWORD& nSamplesPerSec,   // Samples per second
		WORD& wBitsPerSample     // Bits per sample
		)
	{
		if (!m_isOpened)
			return -1;
		nChannels = m_pFormatCtx->streams[m_AudioIndex]->codec->channels;
		nSamplesPerSec = m_pFormatCtx->streams[m_AudioIndex]->codec->sample_rate;
		switch (m_pFormatCtx->streams[m_AudioIndex]->codec->request_sample_fmt)
		{
		case AV_SAMPLE_FMT_U8:			///< unsigned 8 bits
		case AV_SAMPLE_FMT_U8P:         ///< unsigned 8 bits, planar
			wBitsPerSample = 8;
			break;
		case AV_SAMPLE_FMT_S16:         ///< signed 16 bits
		case AV_SAMPLE_FMT_S16P:        ///< signed 16 bits, planar
			wBitsPerSample = 16;
			break;
		case  AV_SAMPLE_FMT_S32:         ///< signed 32 bits
		case AV_SAMPLE_FMT_S32P:        ///< signed 32 bits, planar
			wBitsPerSample = 32;
			break;
		default:
			wBitsPerSample = 0;
			break;
		}
		return 0;
	}