#include "stdafx.h"
#define __STDC_CONSTANT_MACROS
#include <stdio.h>
#include "FFMpegFilter.h"


FFMpegFilter::FFMpegFilter()
{
}


FFMpegFilter::~FFMpegFilter()
{
	cleanup();
}

int FFMpegFilter::cleanup()
{
	if (filter_graph)
	{
		avfilter_graph_free(&filter_graph);
		filter_graph = NULL;
	}
	return 0;
}

int FFMpegFilter::init_filters(const char *filters_descr, AVCodecContext * pCodecCtx)
{
	if (isInited)
		return 0;
    char args[512];
    int ret;
    AVFilter *buffersrc = avfilter_get_by_name("buffer");
    AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
	enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    AVBufferSinkParams *buffersink_params;

    filter_graph = avfilter_graph_alloc();
    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    /*_snprintf_s(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
            pCodecCtx->time_base.num, pCodecCtx->time_base.den,
            pCodecCtx->sample_aspect_ratio.num, pCodecCtx->sample_aspect_ratio.den);*/
	_snprintf_s(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d",
		pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->time_base.num, pCodecCtx->time_base.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                      args, NULL, filter_graph);
    if (ret < 0) {
        printf("Cannot create buffer source\n");
		avfilter_inout_free(&outputs);
		avfilter_inout_free(&inputs);
        return ret;

	}

    /* buffer video sink: to terminate the filter chain. */
    buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts;
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, buffersink_params, filter_graph);
    av_free(buffersink_params);

    if (ret < 0) {
        printf("Cannot create buffer sink\n");
		avfilter_inout_free(&outputs);
		avfilter_inout_free(&inputs);
        return ret;
	}

    /* Endpoints for the filter graph. */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

	if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
		&inputs, &outputs, NULL)) < 0)
	{
		avfilter_inout_free(&outputs);
		avfilter_inout_free(&inputs);
        return ret;
	}

	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
	{
		avfilter_inout_free(&outputs);
		avfilter_inout_free(&inputs);
		return ret;
	}

	isInited = true;
    return 0;
}

AVFrame* FFMpegLogoFilter::AddOneFrame(AVFrame * pFrame)
{
	/* push the decoded frame into the filtergraph */
	if (av_buffersrc_add_frame(buffersrc_ctx, pFrame) < 0) {
		printf("Error while feeding the filtergraph\n");
		return NULL;		
	}
	AVFrame * pResult = av_frame_alloc();
	int ret = av_buffersink_get_frame(buffersink_ctx, pResult);
	if (ret < 0)
	{
		av_frame_free(&pResult);
		return NULL;
	}
	else
		return pResult;
}