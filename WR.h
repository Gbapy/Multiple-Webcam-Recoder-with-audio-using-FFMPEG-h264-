// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the WR_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// WR_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WR_EXPORTS
#define WR_API __declspec(dllexport)
#else
#define WR_API __declspec(dllimport)
#endif

// This class is exported from the WR.dll
class WR_API CWR {
public:
	CWR(void);
	// TODO: add your methods here.
};

extern WR_API int nWR;

WR_API int fnWR(void);

extern "C" {
	BOOL WR_API	TerminateModule();
	bool WR_API GetModuleState();
	int WR_API SetOutputFile(int nCamNo, char *szFile);
	int WR_API	SetDisplayWindow(int nCamNo, HWND hwnd);
	int WR_API SetCameraState(int nCamNo, bool bCamState);
	bool WR_API SetWaterMark(char *szFile);
	int WR_API SetMargin(float nPosX, float nPosY, float nWidth, float nHeight);
	bool WR_API SetWatermarkGeometry(float posX, float posY, float width, float height, float transparency);
}

#pragma once

#include "resource.h"

#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <vector>
#include <CommCtrl.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MMSystem.h>
#include <commdlg.h>
#include <io.h>

extern "C"
{
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
using namespace cv;
using namespace std;

#define UINT64_C(x)  (x ## ULL)
#define MAX_WEBCAM			10
#define INP_BUFFER_SIZE		441
#define STREAM_DURATION		10.0
#define STREAM_FRAME_RATE	25 /* 25 images/s */
#define STREAM_PIX_FMT		AV_PIX_FMT_BGR0 /* default pix_fmt */
#define SCALE_FLAGS SWS_BICUBIC
#define MAX_WSTREAM_COUNT	2

typedef struct OutputStream {
	AVStream *st;
	AVCodecContext *enc;
	/* pts of the next frame that will be generated */
	int64_t next_pts;
	int samples_count;
	AVFrame *frame;
	AVFrame *tmp_frame;
	float t, tincr, tincr2;
	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;
} OutputStream;


typedef struct _WEBCAM_STREAM_ {
	Mat		frm[MAX_WSTREAM_COUNT];
	DWORD	tick[MAX_WSTREAM_COUNT];
	_WEBCAM_STREAM_() {

	}
}WEBCAM_STREAM, *PWEBCAM_STREAM;

static HINSTANCE dll_inst = NULL;


char		szDir[MAX_PATH];
bool		bFoundRecDevice;
bool		bStopFlag = false;

BYTE		lpAudioData[25 * INP_BUFFER_SIZE];
DWORD		nTimeBase;
HWND		hMainDlg;
Mat			waterMark;
int			nResized_Width;
int			nResized_Height;
float		marginPosX;
float		marginPosY;
float		marginWidth;
float		marginHeight;

float		wmPosX;
float		wmPosY;
float		wmWidth;
float		wmHeight;
float		wmTransparency;

int			ScreenDPI = USER_DEFAULT_SCREEN_DPI;
double		DPIScaleFactorX = 1;

static int select_sample_rate(const AVCodec *codec);
static int select_sample_rate(const AVCodec *codec);
static int select_channel_layout(const AVCodec *codec);
static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt);
static bool add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id, int nWidth, int nHeight);
static bool open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
static bool open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
static void close_stream(AVFormatContext *oc, OutputStream *ost);
static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);
static int write_video_frame(AVFormatContext *oc, OutputStream *ost, Mat *frm, DWORD *lastTick);
static int write_audio_frame(AVFormatContext *oc, OutputStream *ost, DWORD *lastTick);
LRESULT ListSubItemSet(HWND list, int index, int subIndex, char *string);

void Warn(HWND hwnd, char *msg);
void ShowImage(HWND hWnd, Mat *frame);
bool FileExists(const char *filePathPtr);

typedef struct _WEBCAM_
{
	int				nWidth;
	int				nHeight;
	bool			bAudioEnabled;
	bool			bStatus;
	bool			bShow;
	int				nVideoBitRate;
	int				nAudioBitRate;
	char			name[MAX_PATH];
	char			szFile[MAX_PATH];
	HANDLE			busy;
	HANDLE			capturing;
	DWORD			nLastTick;
	HWND			hWnd = NULL;

	OutputStream video_st, audio_st;
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVCodec *audio_codec, *video_codec;
	AVDictionary *opt = NULL;

	_WEBCAM_() {

	}

	_WEBCAM_(int nCamNo, int width, int height, bool audioEnabled, int videobRate, int audiobRate) {
		nWidth = width;
		nHeight = height;
		bAudioEnabled = audioEnabled;
		nVideoBitRate = videobRate;
		nAudioBitRate = audiobRate;
		sprintf(name, "Webcam%d", nCamNo);
		bStatus = false;
		bShow = false;
		capturing = CreateEvent(NULL, TRUE, FALSE, name);
		sprintf(szFile, "%s%s.mp4", szDir, name);
		hWnd = NULL;
	}

	bool InitMedia() {
		int ret;

		memset((void *)&video_st, 0, sizeof(OutputStream));
		memset((void *)&audio_st, 0, sizeof(OutputStream));
		/* Initialize libavcodec, and register all codecs and formats. */
		av_register_all();
		AVOutputFormat avo;

		/* allocate the output media context */
		avformat_alloc_output_context2(&oc, NULL, NULL, szFile);
		if (!oc) {
			avformat_alloc_output_context2(&oc, NULL, "mpeg", szFile);
		}
		if (!oc) {
			Warn(hMainDlg, "Could not deduce output format from file extension!");
			return false;
		}

		fmt = oc->oformat;
		/* Add the audio and video streams using the default format codecs
		* and initialize the codecs. */
		if (fmt->video_codec != AV_CODEC_ID_NONE) {
			if (!add_stream(&video_st, oc, &video_codec, fmt->video_codec, nWidth, nHeight)) return false;
		}
		if (fmt->audio_codec != AV_CODEC_ID_NONE) {
			if (!add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec, 0, 0)) return false;
		}
		/* Now that all the parameters are set, we can open the audio and
		* video codecs and allocate the necessary encode buffers. */
		if (!open_video(oc, video_codec, &video_st, opt)) return false;
		if (bFoundRecDevice && bAudioEnabled)
			if (!open_audio(oc, audio_codec, &audio_st, opt)) return false;
		av_dump_format(oc, 0, szFile, 1);
		/* open the output file, if needed */
		if (!(fmt->flags & AVFMT_NOFILE)) {
			ret = avio_open(&oc->pb, szFile, AVIO_FLAG_WRITE);
			if (ret < 0) {
				Warn(hMainDlg, "Could not open MP4 file!");
				return false;
			}
		}
		/* Write the stream header, if any. */
		ret = avformat_write_header(oc, &opt);
		if (ret < 0) {
			if (ret < 0) {
				Warn(hMainDlg, "Could not open MP4 file!");
				return false;
			}
		}
		/* Write the trailer, if any. The trailer must be written before you
		* close the CodecContexts open when you wrote the header; otherwise
		* av_write_trailer() may try to use memory that was freed on
		* av_codec_close(). */
		busy = CreateEvent(NULL, TRUE, TRUE, name);
		SetEvent(busy);
		bStatus = true;
		return true;
	}

	int WriteMedia(int nWebCamNo, Mat *origin) {
		DWORD tick = GetTickCount();
		int ret = 0;
		Mat frm;

		resize(*origin, frm, Size(nWidth, nHeight), CV_INTER_AREA);

		if (!waterMark.empty()) {
			Mat wm;

			resize(waterMark, wm, Size(nResized_Width * wmWidth, nResized_Height * wmHeight));
			for (int i = 0; i < wm.rows; i++) {
				Vec3b *ptr1 = (Vec3b *)wm.ptr(i);
				Vec3b *ptr2 = (Vec3b *)frm.ptr(nResized_Height * wmPosY + i);
				for (int j = 0; j < wm.cols; j++) {
					if (ptr1[j] == Vec3b(0, 0, 0)) continue;
					ptr2[(int)(j + nResized_Width * wmPosX)] = ptr2[(int)(j + nResized_Width * wmPosX)] * (1 - wmTransparency) + ptr1[j] * wmTransparency;
				}
			}
			wm.release();
		}

		if (bStatus) {
			ResetEvent(busy);

			if (!bFoundRecDevice || !bAudioEnabled || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
				audio_st.next_pts, audio_st.enc->time_base) <= 0) {
				if (write_video_frame(oc, &video_st, &frm, &nLastTick) != 0) ret = 2;
			}
			else {
				nLastTick = GetTickCount() - 150;
				if (write_audio_frame(oc, &audio_st, &nLastTick) != 0) ret = 3;
			}
			SetEvent(busy);
		}

		//if (bStatus) {
		//	ListSubItemSet(GetDlgItem(hwnd, IDC_LIST_CAM), nWebCamNo, 1, "Capturing");
		//}
		//else{
		//	ListSubItemSet(GetDlgItem(hwnd, IDC_LIST_CAM), nWebCamNo, 1, "Not-Capturing");
		//}
		if (bShow && hWnd != NULL) {
			ShowImage(hWnd, &frm);
		}
		frm.release();
		return ret;
	}

	void CloseMedia() {
		if (!bStatus) return;
		DWORD dwWaitResult = WaitForSingleObject(busy, INFINITE);
		bStatus = false;
		av_write_trailer(oc);
		/* Close each codec. */
		close_stream(oc, &video_st);
		if (bFoundRecDevice && bAudioEnabled)
			close_stream(oc, &audio_st);
		if (!(fmt->flags & AVFMT_NOFILE))
			/* Close the output file. */
			avio_closep(&oc->pb);
		/* free the stream */
		avformat_free_context(oc);
		CloseHandle(busy);
	}

	void CloseMediaImmediate() {
		if (!bStatus) return;
		bStatus = false;
		av_write_trailer(oc);
		/* Close each codec. */
		close_stream(oc, &video_st);
		if (bFoundRecDevice && bAudioEnabled)
			close_stream(oc, &audio_st);
		if (!(fmt->flags & AVFMT_NOFILE))
			/* Close the output file. */
			avio_closep(&oc->pb);
		/* free the stream */
		avformat_free_context(oc);
		CloseHandle(busy);
	}
}WEBCAM, *PWEMCAM;

VideoCapture	source[MAX_WEBCAM];
WEBCAM			wCam[MAX_WEBCAM];
PWEBCAM_STREAM	wStream[10];
HANDLE			ghRenderThreads[MAX_WEBCAM];
HANDLE			ghCaptureThreads[MAX_WEBCAM];

void ShowImage(HWND hWnd, Mat *frame) {
	Mat frm;
	HDC hdc = GetDC(hWnd);
	HDC hdcMem = CreateCompatibleDC(hdc);
	HGDIOBJ oldBitmap;
	HBITMAP hBmp;
	RECT rect;
	GetClientRect(hWnd, &rect);
	cvtColor(*frame, frm, CV_BGR2BGRA);
	hBmp = CreateBitmap(frm.cols, frm.rows, 1, 8 * frm.channels(), frm.data);
	SetStretchBltMode(hdc, HALFTONE);

	oldBitmap = SelectObject(hdcMem, hBmp);
	int wid = rect.right - rect.left;
	int hig = rect.bottom - rect.top;

	StretchBlt(hdc, wid * marginPosX, hig * marginPosY, wid * marginWidth, hig * marginHeight, hdcMem, 0, 0, frm.cols, frm.rows, SRCCOPY);

	SelectObject(hdcMem, oldBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(hWnd, hdc);
	DeleteDC(hdc);
	DeleteObject(hBmp);
	frm.release();
}

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
	AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
	//printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n");
}

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
	/* rescale output packet timestamp values from codec to stream timebase */
	av_packet_rescale_ts(pkt, *time_base, st->time_base);
	pkt->stream_index = st->index;
	/* Write the compressed frame to the media file. */
	log_packet(fmt_ctx, pkt);
	return av_interleaved_write_frame(fmt_ctx, pkt);
}

static void close_stream(AVFormatContext *oc, OutputStream *ost)
{
	avcodec_free_context(&ost->enc);
	av_frame_free(&ost->frame);
	av_frame_free(&ost->tmp_frame);
	sws_freeContext(ost->sws_ctx);
	swr_free(&ost->swr_ctx);
}

static void fill_yuv_image(AVFrame *pict, int frame_index,
	int width, int height, Mat *frm)
{
	int x, y, i, ret;
	/* when we pass a frame to the encoder, it may keep a reference to it
	* internally;
	* make sure we do not overwrite it here
	*/
	ret = av_frame_make_writable(pict);
	if (ret < 0) {
		Warn(hMainDlg, "Unabled to write video frame!");
		exit(0);
	}
	i = frame_index;
	/* Y */
	for (y = 0; y < height; y++) {
		Vec3b *ptr = (Vec3b *)frm->ptr(y);
		for (x = 0; x < width; x++) {
			pict->data[0][y * pict->linesize[0] + x * 4] = ptr[x].val[0];
			pict->data[0][y * pict->linesize[0] + x * 4 + 1] = ptr[x].val[1];
			pict->data[0][y * pict->linesize[0] + x * 4 + 2] = ptr[x].val[2];
			pict->data[0][y * pict->linesize[0] + x * 4 + 3] = 255;
		}
	}
}

static AVFrame *get_video_frame(OutputStream *ost, Mat *frm)
{
	AVCodecContext *c = ost->enc;
	if (c->pix_fmt == AV_PIX_FMT_YUV420P) {
		/* as we only generate a YUV420P picture, we must convert it
		* to the codec pixel format if needed */
		if (!ost->sws_ctx) {
			ost->sws_ctx = sws_getContext(c->width, c->height,
				AV_PIX_FMT_YUV420P,
				c->width, c->height,
				c->pix_fmt,
				SCALE_FLAGS, NULL, NULL, NULL);
			if (!ost->sws_ctx) {
				Warn(hMainDlg, "Could not initialize the conversion context");
				exit(0);
			}
		}
		fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height, frm);
		sws_scale(ost->sws_ctx,
			(const uint8_t * const *)ost->tmp_frame->data, ost->tmp_frame->linesize,
			0, c->height, ost->frame->data, ost->frame->linesize);
	}
	else {
		fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height, frm);
	}
	ost->frame->pts = ost->next_pts++;
	return ost->frame;
}
/*
* encode one video frame and send it to the muxer
* return 1 when encoding is finished, 0 otherwise
*/
static int write_video_frame(AVFormatContext *oc, OutputStream *ost, Mat *frm, DWORD *lastTick)
{
	int ret = 0;
	AVCodecContext *c;
	AVFrame *frame;
	int got_packet = 0;
	AVPacket pkt;

	c = ost->enc;
	frame = get_video_frame(ost, frm);
	memset((void *)&pkt, 0, sizeof(AVPacket));

	av_init_packet(&pkt);
	ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
	if (ret < 0) {
		Warn(hMainDlg, "Error encoding video frame!");
		return ret;
	}
	if (got_packet) {
		ret = write_frame(oc, &c->time_base, ost->st, &pkt);
	}
	else {
		ret = 0;
	}
	if (ret < 0) {
		Warn(hMainDlg, "Error while writing video frame!");
		return ret;
	}
	return (frame || got_packet) ? 0 : 1;
}

static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
	uint64_t channel_layout,
	int sample_rate, int nb_samples)
{
	AVFrame *frame = av_frame_alloc();
	int ret;
	if (!frame) {
		fprintf(stderr, "Error allocating an audio frame\n");
		exit(1);
	}
	frame->format = sample_fmt;
	frame->channel_layout = channel_layout;
	frame->sample_rate = sample_rate;
	frame->nb_samples = nb_samples;
	if (nb_samples) {
		ret = av_frame_get_buffer(frame, 0);
		if (ret < 0) {
			fprintf(stderr, "Error allocating an audio buffer\n");
			exit(1);
		}
	}
	return frame;
}

static bool open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
	AVCodecContext *c;
	int nb_samples;
	int ret;
	AVDictionary *opt = NULL;
	c = ost->enc;
	/* open it */
	av_dict_copy(&opt, opt_arg, 0);
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		Warn(hMainDlg, "Could not open audio codec!");
		return false;
	}
	/* init signal generator */
	ost->t = 0;
	ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
	/* increment frequency by 110 Hz per second */
	ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;
	if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = c->frame_size;
	ost->frame = alloc_audio_frame(c->sample_fmt, c->channel_layout,
		c->sample_rate, nb_samples);
	ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
		c->sample_rate, nb_samples);
	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) {
		Warn(hMainDlg, "Could not copy the stream parameters!");
		return false;
	}
	/* create resampler context */
	ost->swr_ctx = swr_alloc();
	if (!ost->swr_ctx) {
		Warn(hMainDlg, "Could not allocate resampler context!");
		return false;
	}
	/* set options */
	av_opt_set_int(ost->swr_ctx, "in_channel_count", c->channels, 0);
	av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
	av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	av_opt_set_int(ost->swr_ctx, "out_channel_count", c->channels, 0);
	av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
	av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);
	/* initialize the resampling context */
	if ((ret = swr_init(ost->swr_ctx)) < 0) {
		Warn(hMainDlg, "Failed to initialize the resampling context!");
		return false;
	}
	return true;
}

static AVFrame *get_audio_frame(OutputStream *ost, DWORD *lastTick)
{
	AVFrame *frame = ost->tmp_frame;
	int j, i;
	int16_t *q = (int16_t*)frame->data[0];
	AVRational avr;
	avr.num = 1;
	avr.den = 1;

	/* check if we want to generate more frames */
	//if (av_compare_ts(ost->next_pts, ost->enc->time_base,
	//	STREAM_DURATION, avr) >= 0)
	//	return NULL;

	if (*lastTick < nTimeBase)	*lastTick = nTimeBase;

	DWORD curTick = *lastTick - nTimeBase;
	int curPos = curTick * 11025 / 1000;
	if (curPos + frame->nb_samples < INP_BUFFER_SIZE * 25) {
		for (j = 0; j < frame->nb_samples; j++) {
			//int v = (int)(sin(ost->t) * 10000);
			q[ost->enc->channels * j] = lpAudioData[curPos + j] * 500;
			for (i = 1; i < ost->enc->channels; i++)
			{
				q[ost->enc->channels * j + i] = lpAudioData[curPos + j] * 500;
			}
			ost->t += ost->tincr;
			ost->tincr += ost->tincr2;
		}
		frame->pts = ost->next_pts;
		ost->next_pts += 1024;// frame->nb_samples;
	}
	else
		return NULL;
	return frame;
}
/*
* encode one audio frame and send it to the muxer
* return 1 when encoding is finished, 0 otherwise
*/
static int write_audio_frame(AVFormatContext *oc, OutputStream *ost, DWORD *lastTick)
{
	AVCodecContext *c;
	AVPacket pkt; // data and size must be 0;
	AVFrame *frame;
	int ret = 0;
	int got_packet = 0;
	int dst_nb_samples;
	int rc = 0;

_retry:
	rc++;
	c = ost->enc;
	frame = get_audio_frame(ost, lastTick);
	if (frame) {
		/* convert samples from native format to destination codec format, using the resampler */
		/* compute destination number of samples */
		dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
			c->sample_rate, c->sample_rate, AV_ROUND_UP);
		av_assert0(dst_nb_samples == frame->nb_samples);
		/* when we pass a frame to the encoder, it may keep a reference to it
		* internally;
		* make sure we do not overwrite it here
		*/
		ret = av_frame_make_writable(ost->frame);
		if (ret < 0) {
			Warn(hMainDlg, "Unabled to write audio frame");
			return ret;
		}
		/* convert to destination format */
		ret = swr_convert(ost->swr_ctx,
			ost->frame->data, dst_nb_samples,
			(const uint8_t **)frame->data, frame->nb_samples);
		if (ret < 0) {
			Warn(hMainDlg, "Error while converting");
			return ret;
		}
		frame = ost->frame;
		AVRational avr;
		avr.num = 1;
		avr.den = c->sample_rate;
		frame->pts = av_rescale_q(ost->samples_count, avr, c->time_base);
		ost->samples_count += dst_nb_samples;
	}
	//while (ret >= 0 && !got_packet && frame)
	{
		memset((void *)&pkt, 0, sizeof(AVPacket));
		av_init_packet(&pkt);
		ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
	}
	if (ret < 0) {
		Warn(hMainDlg, "Error encoding audio frame");
		return ret;
	}
	if (got_packet) {
		ret = write_frame(oc, &c->time_base, ost->st, &pkt);
		if (ret < 0) {
			Warn(hMainDlg, "Error while writing audio frame!");
			return ret;
		}
	}
	else{
		ret = 0;
	}
	return (frame || got_packet) ? 0 : 1;
}

static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
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
		Warn(hMainDlg, "Could not allocate frame data!");
		return NULL;
	}
	return picture;
}

static bool open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
	int ret;
	AVCodecContext *c = ost->enc;
	AVDictionary *opt = NULL;
	av_dict_copy(&opt, opt_arg, 0);
	/* open the codec */
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		Warn(hMainDlg, "Could not open video codec");
		return false;
	}
	/* allocate and init a re-usable frame */
	ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
	if (!ost->frame) {
		return false;
	}
	/* If the output format is not YUV420P, then a temporary YUV420P
	* picture is needed too. It is then converted to the required
	* output format. */
	ost->tmp_frame = NULL;
	if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
		ost->tmp_frame = alloc_picture(STREAM_PIX_FMT, c->width, c->height);
		if (!ost->tmp_frame) {
			return false;
		}
	}
	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) {
		Warn(hMainDlg, "Could not copy the stream parameters");
		return false;
	}
	return true;
}

static bool add_stream(OutputStream *ost, AVFormatContext *oc,
	AVCodec **codec, enum AVCodecID codec_id, int nWidth, int nHeight)
{
	AVCodecContext *c;
	int i;
	/* find the encoder */

	if (codec_id == AV_CODEC_ID_H264)
		*codec = avcodec_find_encoder_by_name("libx264rgb");
	else
		*codec = avcodec_find_encoder(codec_id);
	if (!(*codec)) {
		Warn(hMainDlg, "Could not find encoder!");
		return false;
	}

	ost->st = avformat_new_stream(oc, NULL);
	if (!ost->st) {
		Warn(hMainDlg, "Could not allocate stream");
		return false;
	}
	ost->st->id = oc->nb_streams - 1;
	c = avcodec_alloc_context3(*codec);
	if (!c) {
		Warn(hMainDlg, "Could not alloc an encoding context");
		return false;
	}
	ost->enc = c;
	switch ((*codec)->type) {
	case AVMEDIA_TYPE_AUDIO:
		c->sample_fmt = (*codec)->sample_fmts ?
			(*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
		c->bit_rate = 60000;
		c->sample_rate = 11025;
		if ((*codec)->supported_samplerates) {
			c->sample_rate = (*codec)->supported_samplerates[0];
			for (i = 0; (*codec)->supported_samplerates[i]; i++) {
				if ((*codec)->supported_samplerates[i] == 11025)
					c->sample_rate = 11025;
			}
		}
		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
		c->channel_layout = AV_CH_LAYOUT_STEREO;
		if ((*codec)->channel_layouts) {
			c->channel_layout = (*codec)->channel_layouts[0];
			for (i = 0; (*codec)->channel_layouts[i]; i++) {
				if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
					c->channel_layout = AV_CH_LAYOUT_STEREO;
			}
		}
		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
		ost->st->time_base.num = 1;// (AVRational){ 1, c->sample_rate };
		ost->st->time_base.den = c->sample_rate;
		break;
	case AVMEDIA_TYPE_VIDEO:
		c->codec_id = codec_id;
		c->bit_rate = 400000;
		/* Resolution must be a multiple of two. */
		c->width = nWidth;
		c->height = nHeight;
		/* timebase: This is the fundamental unit of time (in seconds) in terms
		* of which frame timestamps are represented. For fixed-fps content,
		* timebase should be 1/framerate and timestamp increments should be
		* identical to 1. */

		ost->st->time_base.num = 1;// (AVRational){ 1, STREAM_FRAME_RATE };
		ost->st->time_base.den = 25;
		c->time_base = ost->st->time_base;
		c->gop_size = 12; /* emit one intra frame every twelve frames at most */
		c->pix_fmt = STREAM_PIX_FMT;

		if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
			/* just for testing, we also add B-frames */
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
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	return true;
}

/* just pick the highest supported samplerate */
static int select_sample_rate(const AVCodec *codec)
{
	const int *p;
	int best_samplerate = 0;

	if (!codec->supported_samplerates)
		return 44100;

	p = codec->supported_samplerates;
	while (*p) {
		if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
			best_samplerate = *p;
		p++;
	}
	return best_samplerate;
}

/* select layout with the highest channel count */
static int select_channel_layout(const AVCodec *codec)
{
	const uint64_t *p;
	uint64_t best_ch_layout = 0;
	int best_nb_channels = 0;

	if (!codec->channel_layouts)
		return AV_CH_LAYOUT_STEREO;

	p = codec->channel_layouts;
	while (*p) {
		int nb_channels = av_get_channel_layout_nb_channels(*p);

		if (nb_channels > best_nb_channels) {
			best_ch_layout = *p;
			best_nb_channels = nb_channels;
		}
		p++;
	}
	return best_ch_layout;
}

static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
	const enum AVSampleFormat *p = codec->sample_fmts;

	while (*p != AV_SAMPLE_FMT_NONE) {
		if (*p == sample_fmt)
			return 1;
		p++;
	}
	return 0;
}

int CompensateXDPI(int val)
{
	if (ScreenDPI == USER_DEFAULT_SCREEN_DPI)
		return val;
	else
	{
		double tmpVal = (double)val * DPIScaleFactorX;

		if (tmpVal > 0)
			return (int)floor(tmpVal);
		else
			return (int)ceil(tmpVal);
	}
}

LRESULT ListItemAdd(HWND list, int index, char *string)
{
	LVITEM li;
	memset(&li, 0, sizeof(li));

	li.mask = LVIF_TEXT;
	li.pszText = string;
	li.iItem = index;
	li.iSubItem = 0;
	return ListView_InsertItem(list, &li);
}

LRESULT ListSubItemSet(HWND list, int index, int subIndex, char *string)
{
	LVITEM li;
	memset(&li, 0, sizeof(li));

	li.mask = LVIF_TEXT;
	li.pszText = string;
	li.iItem = index;
	li.iSubItem = subIndex;
	return ListView_SetItem(list, &li);
}

void Warn(HWND hwnd, char *msg) {
	MessageBox(hwnd, msg, "warn", MB_ICONEXCLAMATION | MB_OK);
}


bool FileExists(const char *filePathPtr)
{
	char filePath[_MAX_PATH];

	// Strip quotation marks (if any)
	if (filePathPtr[0] == '"')
	{
		strcpy(filePath, filePathPtr + 1);
	}
	else
	{
		strcpy(filePath, filePathPtr);
	}

	// Strip quotation marks (if any)
	if (filePath[strlen(filePath) - 1] == '"')
		filePath[strlen(filePath) - 1] = 0;

	return (_access(filePath, 0) != -1);
}

