#pragma once

extern "C"
{
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
}

class FFMpegFilter
{
protected:	
	AVFilterContext *buffersink_ctx;
	AVFilterContext *buffersrc_ctx;
	AVFilterGraph *filter_graph = NULL;
	bool isInited = false;
	AVFilterContext* volume_ctx;
	AVFilter*		volume;
public:
	FFMpegFilter();
	virtual ~FFMpegFilter();
	int init_filters(const char *filters_descr, AVCodecContext * pCodecCtx);
	int cleanup();

};

class FFMpegLogoFilter : public FFMpegFilter
{
	//'overlay=10:main_h-overlay_h-10'
	const char *filter_descr = "movie=TMark.png[wm];[in][wm]overlay=main_w-overlay_w-20:20";
public:
	FFMpegLogoFilter(){};
	virtual ~FFMpegLogoFilter(){};
	int init(AVCodecContext * pCodecCtx) 
	{
		return init_filters(filter_descr, pCodecCtx);
	};
	bool IsInited(){ return isInited; }
	//the obtained Frame need to be freeed later
	AVFrame* AddOneFrame(AVFrame * pFrame);
};