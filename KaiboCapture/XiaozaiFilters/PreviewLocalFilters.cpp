// XiaozaiFilters.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <map>
#include "PreviewLocalFilters.h"
#include <objidl.h>
#include <gdiplus.h>
extern "C" {
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavfilter/avfiltergraph.h"
}
#include "FFMpegInputWapper.h"
#include "FFMpegDecoder.h"
#include "ComHelper.h"
#include "previewLocalController.h"



static Gdiplus::GdiplusStartupInput gdiplusStartupInput;
static ULONG_PTR           gdiplusToken;

static FILE *stream = NULL;


static void logFF(void *, int level, const char *fmt, va_list ap)
{
	if (level <= av_log_get_level())
		vfprintf(stderr, fmt, ap);
}

static void logFF(int level, const char *fmt, ...)
{
	if (level <= av_log_get_level())
	{
		va_list   pArgList;
		va_start(pArgList, fmt);
		vfprintf(stderr, fmt, pArgList);
		va_end(pArgList);
	}

}


bool CPreviewLocalFilters::Init(int loglevel)
{
	freopen_s(&stream,"log.txt", "w", stderr);
	//AllocConsole();
	setlocale(LC_ALL, "zh-CN");
	
	av_log_set_level(loglevel);
	av_register_all();
	avformat_network_init();
	avdevice_register_all();
	avfilter_register_all();

	av_log_set_callback(logFF);
	
	// Initialize GDI+.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	return true;
}

bool CPreviewLocalFilters::Uninit()
{
	GdiplusShutdown(gdiplusToken);
	if (stream)
		fclose(stream);
	return true;
}

// This is the constructor of a class that has been exported.
// see XiaozaiFilters.h for the class definition
CPreviewLocalFilters::CPreviewLocalFilters()
{
	m_SuperSetting = new PreviewLocalController();
	return;
}

CPreviewLocalFilters::~CPreviewLocalFilters()
{
	CPreviewLocalFilters *pController = (CPreviewLocalFilters *)m_SuperSetting;
	//SAFE_DELETE(pController);
}

int CPreviewLocalFilters::Test(HWND hwnd)
{
	Init(XZ_LOG_VERBOSE);
	FFMpegInputWapper myinput;
	std::map<const char*, const char*> mymap;
	mymap.insert(std::pair<const char*, const char*>("loglevel", "trace"));
	myinput.set_options(mymap, 0);
	myinput.open_input_device("video=Integrated Camera:audio=Microphone (Realtek High Definition Audio)");
	FFMpegVideoDecoder myvdecoder;
	myvdecoder.open_codec(myinput.get_stream_codec(myinput.get_videostream_index()));
	FFMpegAudioDecoder myadecoder;
	myadecoder.open_codec(myinput.get_stream_codec(myinput.get_audiostream_index()));
	AVPacket * mupacket = NULL;
	Gdiplus::Graphics* mygraphic = new Gdiplus::Graphics(hwnd, FALSE);
	int i = 0;
	do {
		mupacket = myinput.get_a_frame();
		if (mupacket && (mupacket->stream_index != myinput.get_videostream_index()))
		{
			AVFrame *myframe = myadecoder.decode_with_resample(mupacket, 0, AV_SAMPLE_FMT_S16, 0);
		
			av_frame_unref(myframe);
			continue;
		}

		if (mupacket == NULL)
			break;
		Bitmap * bmp = myvdecoder.decode2BMP(mupacket, 0, 0);
		if (bmp)
			mygraphic->DrawImage(bmp,0,0);
		SAFE_DELETE(bmp);
		if (mupacket)
			av_packet_unref(mupacket);
	} while (true);

	AVPacket packet;
	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;
	while (true)
	{
		Bitmap * bmp = myvdecoder.decode2BMP(&packet, 0, 0);
		if (bmp)
			mygraphic->DrawImage(bmp, 0, 0);
		else
			break;
		SAFE_DELETE(bmp);
	}
	SAFE_DELETE(mygraphic);
	Uninit();
	return 0;
}
