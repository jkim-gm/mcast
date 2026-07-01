// exMcPlayer.cpp
//

// d:\d_doc\jkim\ffmpeg.exe
// ffmpeg -f gdigrab -framerate 30 -offset_x 0 -offset_y 0 -video_size 640x480 -i desktop -c:v libx264 -preset ultrafast -tune zerolatency -g 30 -keyint_min 30 -sc_threshold 0 -flags +cgo -f mpegts udp://239.10.10.10:6000

#include "pbsdl.h"
#include <windows.h>
#include <iostream>
#include <stdarg.h>

extern "C" {
#ifndef inline
#define inline __inline
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/common.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libavutil/time.h>
#include <libavutil/avstring.h>

#ifdef _MSC_VER
#ifndef _STDINT
	typedef __int32 int32_t;
	typedef unsigned __int32 uint32_t;
	typedef __int64 int64_t;
	typedef unsigned __int64 uint64_t;
	typedef signed char int8_t;
	typedef unsigned char uint8_t;
	typedef short int16_t;
	typedef unsigned short uint16_t;
#endif
#else
#include <stdint.h>
#endif
}

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_thread.h>
#pragma comment (lib, "SDL2.lib")

#define _MAIN_ENTRY
#define _EN_EVENT_LOOP


#ifdef _DEBUG
#define dmsg(m,...)		__dmsg(__FUNCTION__, m, __VA_ARGS__)
#else
#define dmsg(m,...)
#endif

void __dmsg(const char* _f, const char* _m, ...)
{
	va_list vl;
	va_start(vl, _m);
	printf("(%s) ", _f);
	vprintf(_m, vl);
	
	char msg[256] = { 0	};
	sprintf_s(msg, "(%s) ", _f);
	int  len = strlen(msg);	
	vsprintf_s((char*)(msg + len), 256-len, _m, vl);
	va_end(vl);

	OutputDebugStringA(msg);
}

SDL_Window*		m_sdlWnd = NULL;
SDL_Surface*	m_sdlSurf = NULL;
SDL_Renderer*	m_sdlRend = NULL;
SDL_Texture*	m_sdlTex = NULL;
SDL_RendererInfo m_sdlRendInfo = { 0 };
HWND m_hParWnd = NULL;

int sdl2_init(int x, int y, int wx, int wy, HWND hParWnd)
{
	dmsg("start\n");

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)!=0) {
		dmsg("failed\n");
		return -1;
	}
	
	SDL_version sdlVer;
	SDL_GetVersion(&sdlVer);
	dmsg("SDL version %d.%d.%d\n", sdlVer.major, sdlVer.minor, sdlVer.patch);

	if(hParWnd!=NULL) {
		m_sdlWnd = SDL_CreateWindowFrom(hParWnd);
	}
	else {
		m_sdlWnd = SDL_CreateWindow("sdl2", x, y, wx, wy, 0);
	}
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
	SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "direct3d11", SDL_HINT_OVERRIDE);

	if(!m_sdlWnd) {
		return -1;
	}

	for (int i = 0; i < SDL_GetNumRenderDrivers(); ++i)
	{
		if (SDL_GetRenderDriverInfo(i, &m_sdlRendInfo) != 0) {
			dmsg("SDL_GetRendererInfo failed\n");
		}
		dmsg("SDL_Renderer=%s\n", m_sdlRendInfo.name);
	}

	Uint32 flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
	m_sdlRend = SDL_CreateRenderer(m_sdlWnd, -1, flags);
	if (!m_sdlRend) {
		return -1;
	}

	if (SDL_GetRendererInfo(m_sdlRend, &m_sdlRendInfo)!=0) {
		dmsg("SDL_GetRendererInfo failed\n");
	}
	dmsg("SDL_Renderer=%s", m_sdlRendInfo.name);
#if 0
	m_sdlSurf = IMG_Load("path");
	m_sdlTex = SDL_CreateTextureFromSurface(m_sdlRend, m_sdlSurf);
	if (m_sdlSurf) {
		SDL_FreeSurface(m_sdlSurf);
		m_sdlSurf = NULL;
	}
	SDL_Rect dest;
	SDL_QueryTexture(tex, NULL, NULL, &dest.w, &dest.h);
#endif

	return 0;
}

void sdl2_begin()
{
	if (m_sdlRend) {
		SDL_RenderClear(m_sdlRend);
	}
}

void sdl2_rectangle(SDL_Rect rt, Uint8 a, Uint8 r, Uint8 g, Uint8 b, bool bFill)
{
	if (m_sdlRend) {
		SDL_SetRenderDrawBlendMode(m_sdlRend, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(m_sdlRend, r, g, b, a);
		if(bFill)
			SDL_RenderFillRect(m_sdlRend, &rt);
		else
			SDL_RenderDrawRect(m_sdlRend, &rt);
	}
}

void sdl2_end()
{
	if (m_sdlRend) {
		SDL_RenderPresent(m_sdlRend);
	}
}

void sdl2_release()
{
	if (m_sdlTex) {
		SDL_DestroyTexture(m_sdlTex);
		m_sdlTex = NULL;
	}
	if (m_sdlRend) {
		SDL_DestroyRenderer(m_sdlRend);
		m_sdlRend = NULL;
	}
	if (m_sdlWnd) {
		SDL_DestroyWindow(m_sdlWnd);
		m_sdlWnd = NULL;
	}
	SDL_Quit();
}

void sld2_loop(int* piLoop)
{
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

#if 0
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif
	SDL_Event sdlEvt;

	for (;*piLoop;) {

		double remaining_time = 0.0;
		SDL_PumpEvents();
		while (!SDL_PeepEvents(&sdlEvt, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
			/*
			if (remaining_time > 0.0)
				av_usleep((int64_t)(remaining_time * 1000000.0));
			remaining_time = REFRESH_RATE;
			if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
				video_refresh(is, &remaining_time);
			*/
			SDL_PumpEvents();

			if (!*piLoop)
				break;
		}

		switch (sdlEvt.type) {
		case SDL_KEYDOWN:
			if (sdlEvt.key.keysym.sym == SDLK_ESCAPE || sdlEvt.key.keysym.sym == SDLK_q) {
				break;
			}
			break;
		case SDL_QUIT:
			break;
		default:
			break;
		}
	}
}

#define FF_QUIT_EVENT            (SDL_USEREVENT + 2)
#define FF_JBUF_BUFFERING_EVENT  (SDL_USEREVENT + 3)
#define FF_JBUF_READY_EVENT      (SDL_USEREVENT + 4)

#ifdef _DEBUG
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#else
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#endif
#define MIN_FRAMES 25
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* Step size for volume control in dB */
#define SDL_VOLUME_STEP (0.75)

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

#define CURSOR_HIDE_DELAY 1000000

#define USE_ONEPASS_SUBTITLE_RENDER 1

static unsigned sws_flags = SWS_BICUBIC;

typedef struct MyAVPacketList {
	AVPacket pkt;
	struct MyAVPacketList *next;
	int serial;
} MyAVPacketList;

typedef struct PacketQueue {
	MyAVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	int64_t duration;
	int abort_request;
	int serial;
	SDL_mutex *mutex;
	SDL_cond *cond;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

typedef struct AudioParams {
	int freq;
	int channels;
	int64_t channel_layout;
	enum AVSampleFormat fmt;
	int frame_size;
	int bytes_per_sec;
} AudioParams;

typedef struct Clock {
	double pts;           /* clock base */
	double pts_drift;     /* clock base minus time at which we updated the clock */
	double last_updated;
	double speed;
	int serial;           /* clock is based on a packet with this serial */
	int paused;
	int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame {
	AVFrame *frame;
	AVSubtitle sub;
	int serial;
	double pts;           /* presentation timestamp for the frame */
	double duration;      /* estimated duration of the frame */
	int64_t pos;          /* byte position of the frame in the input file */
	int width;
	int height;
	int format;
	AVRational sar;
	int uploaded;
	int flip_v;
} Frame;

typedef struct FrameQueue {
	Frame queue[FRAME_QUEUE_SIZE];
	int rindex;
	int windex;
	int size;
	int max_size;
	int keep_last;
	int rindex_shown;
	SDL_mutex *mutex;
	SDL_cond *cond;
	PacketQueue *pktq;
} FrameQueue;

enum {
	AV_SYNC_AUDIO_MASTER, /* default choice */
	AV_SYNC_VIDEO_MASTER,
	AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct Decoder {
	AVPacket pkt;
	PacketQueue *queue;
	AVCodecContext *avctx;
	int pkt_serial;
	int finished;
	int packet_pending;
	SDL_cond *empty_queue_cond;
	int64_t start_pts;
	AVRational start_pts_tb;
	int64_t next_pts;
	AVRational next_pts_tb;
	SDL_Thread *decoder_tid;
} Decoder;

enum ShowMode {
	SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
};

typedef struct VideoState {
	SDL_Thread *read_tid;
	SDL_Thread *event_tid;
	SDL_Thread *monitor_tid;
	AVInputFormat *iformat;
	int abort_request;
	int force_refresh;
	int paused;
	int last_paused;
	int queue_attachments_req;
	int seek_req;
	int seek_flags;
	int64_t seek_pos;
	int64_t seek_rel;
	int read_pause_return;
	AVFormatContext *ic;
	int realtime;

	Clock audclk;
	Clock vidclk;
	Clock extclk;

	FrameQueue pictq;
	FrameQueue subpq;
	FrameQueue sampq;

	Decoder auddec;
	Decoder viddec;
	Decoder subdec;

	int audio_stream;

	int av_sync_type;

	double audio_clock;
	int audio_clock_serial;
	double audio_diff_cum; /* used for AV difference average computation */
	double audio_diff_avg_coef;
	double audio_diff_threshold;
	int audio_diff_avg_count;
	AVStream *audio_st;
	PacketQueue audioq;
	int audio_hw_buf_size;
	uint8_t *audio_buf;
	uint8_t *audio_buf1;
	unsigned int audio_buf_size; /* in bytes */
	unsigned int audio_buf1_size;
	int audio_buf_index; /* in bytes */
	int audio_write_buf_size;
	int audio_volume;
	int muted;
	struct AudioParams audio_src;
#if CONFIG_AVFILTER
	struct AudioParams audio_filter_src;
#endif
	struct AudioParams audio_tgt;
	struct SwrContext *swr_ctx;
	int frame_drops_early;
	int frame_drops_late;
		
	enum ShowMode show_mode;
	int16_t sample_array[SAMPLE_ARRAY_SIZE];
	int sample_array_index;
	int last_i_start;
	//RDFTContext *rdft;
	//int rdft_bits;
	//FFTSample *rdft_data;
	int xpos;
	double last_vis_time;
	SDL_Texture *vis_texture;
	SDL_Texture *sub_texture;
	SDL_Texture *vid_texture;

	int subtitle_stream;
	AVStream *subtitle_st;
	PacketQueue subtitleq;

	double frame_timer;
	double frame_last_returned_time;
	double frame_last_filter_delay;
	int video_stream;
	AVStream *video_st;
	PacketQueue videoq;
	double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
	struct SwsContext *img_convert_ctx;
	struct SwsContext *sub_convert_ctx;
	int eof;

	char *filename;

	SDL_Rect rect;
	int imgw, imgh;
	int vp_width = -1;
	int vp_height;
	AVRational vp_sar;

	int width, height, xleft, ytop;
	int step;

#if CONFIG_AVFILTER
	int vfilter_idx;
	AVFilterContext *in_video_filter;   // the first filter in the video chain
	AVFilterContext *out_video_filter;  // the last filter in the video chain
	AVFilterContext *in_audio_filter;   // the first filter in the audio chain
	AVFilterContext *out_audio_filter;  // the last filter in the audio chain
	AVFilterGraph *agraph;              // audio filter graph
#endif

	int last_video_stream, last_audio_stream, last_subtitle_stream;

	SDL_cond *continue_read_thread;

	// Jitter buffer state
	volatile int prebuffering;  // 1 = filling jitter buffer (output frozen)
	int user_paused;            // 1 = user explicitly paused (don't auto-resume)
	int jbuf_eff_max_bytes;     // effective byte cap (auto-computed from bitrate × target)
	volatile int resync_req;    // 1 = on resume, flush stale backlog and re-buffer from live

} VideoState;

static const struct TextureFormatEntry {
	enum AVPixelFormat format;
	int texture_fmt;
} sdl_texture_format_map[] = {
	{ AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
	{ AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
	{ AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
	{ AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
	{ AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
	{ AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
	{ AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
	{ AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
	{ AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
	{ AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
	{ AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
	{ AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
	{ AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
	{ AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
	{ AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
	{ AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
	{ AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
	{ AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
	{ AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
	{ AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN },
};

AVInputFormat *m_avInputFmt = NULL;
AVPacket	flush_pkt;
int			m_iVolume = 128;
SDL_AudioDeviceID audio_dev;
int			decoder_reorder_pts = -1;
int			is_full_screen = 0;
int screen_left = SDL_WINDOWPOS_CENTERED;
int screen_top = SDL_WINDOWPOS_CENTERED;

int default_width = 640;
int default_height = 480;
int screen_width = 0;
int screen_height = 0;
int display_disable = 0;
int video_disable = 0;
int audio_disable = 0;
int cursor_hidden = 0;
int64_t cursor_last_shown = 0;
int64_t audio_callback_time = 0;
int seek_by_bytes = -1;
double rdftspeed = 0.02;
int framedrop = -1;
int show_status = -1;
int lowres = 0;
int fast = 0;
float seek_interval = 10;
int borderless = 1;
int alwaysontop = 0;
int64_t start_time = AV_NOPTS_VALUE;
int64_t duration = AV_NOPTS_VALUE;
int infinite_buffer = -1;
int loop = 1;
int genpts = 0;
int autoexit = 0;
enum ShowMode show_mode = SHOW_MODE_NONE;

VideoState *m_is = NULL;
pbsdl_rendercallback rendercallback = NULL;

// Jitter buffer configuration (set via pbsdl_jbuf_config before pbsdl_load)
int    g_jbuf_enable		= 1;           // 0 = disabled, 1 = enabled
double g_jbuf_target_sec	= 1.0;         // target pre-buffer duration (seconds)
#ifdef _DEBUG
int    g_jbuf_max_bytes		= 4*1024*1024; // max packet queue size before pausing read (4 MB)
#else
int    g_jbuf_max_bytes     = 4*1024*1024; // max packet queue size before pausing read (4 MB)
#endif
#ifdef _DEBUG
double g_jbuf_reopen_ratio	= 0.5;        // re-buffer when queue drops below this ratio of target
#else
double g_jbuf_reopen_ratio  = 0.3;        // re-buffer when queue drops below this ratio of target
#endif
void* cb_receiver = NULL;

int mJumpTo = 0;
int mSkipFrame = 0;
int mTargetFrame = 0;

void stream_toggle_pause(VideoState *is);
void set_clock(Clock *c, double pts, int serial);

AVDictionary *format_opts, *codec_opts;

//-----------------------------------------------------------------------------
// packet
//-----------------------------------------------------------------------------
inline int cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1, enum AVSampleFormat fmt2, int64_t channel_count2)
{
	/* If channel count == 1, planar and non-planar formats are the same */
	if (channel_count1 == 1 && channel_count2 == 1)
		return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
	else
		return channel_count1 != channel_count2 || fmt1 != fmt2;
}

inline int64_t get_valid_channel_layout(int64_t channel_layout, int channels)
{
	if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels)
		return channel_layout;
	else
		return 0;
}

inline int compute_mod(int a, int b)
{
	return a < 0 ? a % b + b : a % b;
}


int packet_queue_put_private(PacketQueue *q, AVPacket *pkt)
{
	MyAVPacketList *pkt1;

	if (q->abort_request)
		return -1;

	pkt1 = (MyAVPacketList*)av_malloc(sizeof(MyAVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;
	if (pkt == &flush_pkt)
		q->serial++;
	pkt1->serial = q->serial;

	if (!q->last_pkt)
		q->first_pkt = pkt1;
	else
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size + sizeof(*pkt1);
	q->duration += pkt1->pkt.duration;
	/* XXX: should duplicate packet data in DV case */
	SDL_CondSignal(q->cond);
	return 0;
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
	int ret;

	SDL_LockMutex(q->mutex);
	ret = packet_queue_put_private(q, pkt);
	SDL_UnlockMutex(q->mutex);

	if (pkt != &flush_pkt && ret < 0)
		av_packet_unref(pkt);

	return ret;
}

int packet_queue_put_nullpacket(PacketQueue *q, int stream_index)
{
	AVPacket pkt1, *pkt = &pkt1;
	av_init_packet(pkt);
	pkt->data = NULL;
	pkt->size = 0;
	pkt->stream_index = stream_index;
	return packet_queue_put(q, pkt);
}

int packet_queue_init(PacketQueue *q)
{
	memset(q, 0, sizeof(PacketQueue));
	q->mutex = SDL_CreateMutex();
	if (!q->mutex) {
		dmsg("SDL_CreateMutex(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
	q->cond = SDL_CreateCond();
	if (!q->cond) {
		dmsg("SDL_CreateCond(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
	q->abort_request = 1;
	return 0;
}

void packet_queue_flush(PacketQueue *q)
{
	MyAVPacketList *pkt, *pkt1;

	SDL_LockMutex(q->mutex);
	for (pkt = q->first_pkt; pkt; pkt = pkt1) {
		pkt1 = pkt->next;
		av_packet_unref(&pkt->pkt);
		av_freep(&pkt);
	}
	q->last_pkt = NULL;
	q->first_pkt = NULL;
	q->nb_packets = 0;
	q->size = 0;
	q->duration = 0;
	SDL_UnlockMutex(q->mutex);
}

void packet_queue_destroy(PacketQueue *q)
{
	packet_queue_flush(q);
	SDL_DestroyMutex(q->mutex);
	SDL_DestroyCond(q->cond);
}

void packet_queue_abort(PacketQueue *q)
{
	SDL_LockMutex(q->mutex);
	q->abort_request = 1;
	SDL_CondSignal(q->cond);
	SDL_UnlockMutex(q->mutex);
}

void packet_queue_start(PacketQueue *q)
{
	SDL_LockMutex(q->mutex);
	q->abort_request = 0;
	packet_queue_put_private(q, &flush_pkt);
	SDL_UnlockMutex(q->mutex);
}

int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial)
{
	MyAVPacketList *pkt1;
	int ret;

	SDL_LockMutex(q->mutex);

	for (;;) {
		if (q->abort_request) {
			ret = -1;
			break;
		}

		pkt1 = q->first_pkt;
		if (pkt1) {
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size + sizeof(*pkt1);
			q->duration -= pkt1->pkt.duration;
			*pkt = pkt1->pkt;
			if (serial)
				*serial = pkt1->serial;
			av_free(pkt1);
			ret = 1;
			break;
		}
		else if (!block) {
			ret = 0;
			break;
		}
		else {
			SDL_CondWait(q->cond, q->mutex);
		}
	}
	SDL_UnlockMutex(q->mutex);
	return ret;
}

//-----------------------------------------------------------------------------
// decoder
//-----------------------------------------------------------------------------
void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond)
{
	memset(d, 0, sizeof(Decoder));
	d->avctx = avctx;
	d->queue = queue;
	d->empty_queue_cond = empty_queue_cond;
	d->start_pts = AV_NOPTS_VALUE;
	d->pkt_serial = -1;
}

int decoder_decode_frame(Decoder *d, AVFrame *frame, AVSubtitle *sub)
{
	int ret = AVERROR(EAGAIN);

	for (;;) {
		AVPacket pkt;

		if (d->queue->serial == d->pkt_serial) {
			do {
				if (d->queue->abort_request)
					return -1;

				switch (d->avctx->codec_type) {
				case AVMEDIA_TYPE_VIDEO:
					ret = avcodec_receive_frame(d->avctx, frame);
					if (ret >= 0) {
						if (decoder_reorder_pts == -1) {
							frame->pts = frame->best_effort_timestamp;
						}
						else if (!decoder_reorder_pts) {
							frame->pts = frame->pkt_dts;
						}
					}
					break;
				case AVMEDIA_TYPE_AUDIO:
					ret = avcodec_receive_frame(d->avctx, frame);
					if (ret >= 0) {
						AVRational tb = { 1, frame->sample_rate };
						if (frame->pts != AV_NOPTS_VALUE)
							frame->pts = av_rescale_q(frame->pts, d->avctx->pkt_timebase, tb);
						else if (d->next_pts != AV_NOPTS_VALUE)
							frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
						if (frame->pts != AV_NOPTS_VALUE) {
							d->next_pts = frame->pts + frame->nb_samples;
							d->next_pts_tb = tb;
						}
					}
					break;
				}
				if (ret == AVERROR_EOF) {
					d->finished = d->pkt_serial;
					avcodec_flush_buffers(d->avctx);
					return 0;
				}
				if (ret >= 0)
					return 1;
			} while (ret != AVERROR(EAGAIN));
		}

		do {
			if (d->queue->nb_packets == 0)
				SDL_CondSignal(d->empty_queue_cond);
			if (d->packet_pending) {
				av_packet_move_ref(&pkt, &d->pkt);
				d->packet_pending = 0;
			}
			else {
				if (packet_queue_get(d->queue, &pkt, 1, &d->pkt_serial) < 0)
					return -1;
			}
			if (d->queue->serial == d->pkt_serial)
				break;
			av_packet_unref(&pkt);
		} while (1);

		if (pkt.data == flush_pkt.data) {
			avcodec_flush_buffers(d->avctx);
			d->finished = 0;
			d->next_pts = d->start_pts;
			d->next_pts_tb = d->start_pts_tb;
		}
		else {
			if (d->avctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
				int got_frame = 0;
				ret = avcodec_decode_subtitle2(d->avctx, sub, &got_frame, &pkt);
				if (ret < 0) {
					ret = AVERROR(EAGAIN);
				}
				else {
					if (got_frame && !pkt.data) {
						d->packet_pending = 1;
						av_packet_move_ref(&d->pkt, &pkt);
					}
					ret = got_frame ? 0 : (pkt.data ? AVERROR(EAGAIN) : AVERROR_EOF);
				}
			}
			else {
				if (avcodec_send_packet(d->avctx, &pkt) == AVERROR(EAGAIN)) {
					av_log(d->avctx, AV_LOG_ERROR, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
					d->packet_pending = 1;
					av_packet_move_ref(&d->pkt, &pkt);
				}
			}
			av_packet_unref(&pkt);
		}
	}
}

void decoder_destroy(Decoder *d)
{
	av_packet_unref(&d->pkt);
	avcodec_free_context(&d->avctx);
}

void frame_queue_signal(FrameQueue *f);

void decoder_abort(Decoder *d, FrameQueue *fq)
{
	packet_queue_abort(d->queue);
	frame_queue_signal(fq);
	SDL_WaitThread(d->decoder_tid, NULL);
	d->decoder_tid = NULL;
	packet_queue_flush(d->queue);
}

//-----------------------------------------------------------------------------
// frame
//-----------------------------------------------------------------------------
void frame_queue_unref_item(Frame *vp)
{
	av_frame_unref(vp->frame);
	avsubtitle_free(&vp->sub);
}

int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last)
{
	int i;
	memset(f, 0, sizeof(FrameQueue));
	if (!(f->mutex = SDL_CreateMutex())) {
		dmsg("SDL_CreateMutex(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
	if (!(f->cond = SDL_CreateCond())) {
		dmsg("SDL_CreateCond(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
	f->pktq = pktq;
	f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
	f->keep_last = !!keep_last;
	for (i = 0; i < f->max_size; i++)
		if (!(f->queue[i].frame = av_frame_alloc()))
			return AVERROR(ENOMEM);
	return 0;
}

void frame_queue_destory(FrameQueue *f)
{
	int i;
	for (i = 0; i < f->max_size; i++) {
		Frame *vp = &f->queue[i];
		frame_queue_unref_item(vp);
		av_frame_free(&vp->frame);
	}
	SDL_DestroyMutex(f->mutex);
	SDL_DestroyCond(f->cond);
}

void frame_queue_signal(FrameQueue *f)
{
	SDL_LockMutex(f->mutex);
	SDL_CondSignal(f->cond);
	SDL_UnlockMutex(f->mutex);
}

Frame *frame_queue_peek(FrameQueue *f)
{
	return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

Frame *frame_queue_peek_next(FrameQueue *f)
{
	return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

Frame *frame_queue_peek_last(FrameQueue *f)
{
	return &f->queue[f->rindex];
}

Frame *frame_queue_peek_writable(FrameQueue *f)
{
	/* wait until we have space to put a new frame */
	SDL_LockMutex(f->mutex);
	while (f->size >= f->max_size &&
		!f->pktq->abort_request) {
		SDL_CondWait(f->cond, f->mutex);
	}
	SDL_UnlockMutex(f->mutex);

	if (f->pktq->abort_request)
		return NULL;

	return &f->queue[f->windex];
}

Frame *frame_queue_peek_readable(FrameQueue *f)
{
	/* wait until we have a readable a new frame */
	SDL_LockMutex(f->mutex);
	while (f->size - f->rindex_shown <= 0 &&
		!f->pktq->abort_request) {
		SDL_CondWait(f->cond, f->mutex);
	}
	SDL_UnlockMutex(f->mutex);

	if (f->pktq->abort_request)
		return NULL;

	return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

void frame_queue_push(FrameQueue *f)
{
	if (++f->windex == f->max_size)
		f->windex = 0;
	SDL_LockMutex(f->mutex);
	f->size++;
	SDL_CondSignal(f->cond);
	SDL_UnlockMutex(f->mutex);
}

void frame_queue_next(FrameQueue *f)
{
	if (f->keep_last && !f->rindex_shown) {
		f->rindex_shown = 1;
		return;
	}
	frame_queue_unref_item(&f->queue[f->rindex]);
	if (++f->rindex == f->max_size)
		f->rindex = 0;
	SDL_LockMutex(f->mutex);
	f->size--;
	SDL_CondSignal(f->cond);
	SDL_UnlockMutex(f->mutex);
}

/* return the number of undisplayed frames in the queue */
int frame_queue_nb_remaining(FrameQueue *f)
{
	return f->size - f->rindex_shown;
}

/* return last shown position */
int64_t frame_queue_last_pos(FrameQueue *f)
{
	Frame *fp = &f->queue[f->rindex];
	if (f->rindex_shown && fp->serial == f->pktq->serial)
		return fp->pos;
	else
		return -1;
}

double frame_queue_last_pts(FrameQueue *f)
{
	Frame *fp = &f->queue[f->rindex];
	if (f->rindex_shown && fp->serial == f->pktq->serial)
		return fp->pts;
	else
		return -1;
}

//-----------------------------------------------------------------------------
// ???
//-----------------------------------------------------------------------------
inline void fill_rectangle(int x, int y, int w, int h)
{
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	if (w && h) {
		SDL_RenderFillRect(m_sdlRend, &rect);
	}
}

int realloc_texture(SDL_Texture **texture, Uint32 new_format, int new_width, int new_height, SDL_BlendMode blendmode, int init_texture)
{
	Uint32 format;
	int access, w, h;
	if (!*texture || SDL_QueryTexture(*texture, &format, &access, &w, &h) < 0 || new_width != w || new_height != h || new_format != format) {
		void *pixels;
		int pitch;
		if (*texture)
			SDL_DestroyTexture(*texture);
		if (!(*texture = SDL_CreateTexture(m_sdlRend, new_format, SDL_TEXTUREACCESS_STREAMING, new_width, new_height)))
			return -1;
		if (SDL_SetTextureBlendMode(*texture, blendmode) < 0)
			return -1;
		if (init_texture) {
			if (SDL_LockTexture(*texture, NULL, &pixels, &pitch) < 0)
				return -1;
			memset(pixels, 0, pitch * new_height);
			SDL_UnlockTexture(*texture);
		}
		dmsg("Created %dx%d texture with %s.\n", new_width, new_height, SDL_GetPixelFormatName(new_format));
	}
	return 0;
}

void calculate_display_rect(SDL_Rect *rect,
	int scr_xleft, int scr_ytop, int scr_width, int scr_height,
	int pic_width, int pic_height, AVRational pic_sar)
{
	AVRational aspect_ratio = pic_sar;
	int64_t width, height, x, y;

	if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
		aspect_ratio = av_make_q(1, 1);

	aspect_ratio = av_mul_q(aspect_ratio, av_make_q(pic_width, pic_height));

	/* XXX: we suppose the screen has a 1.0 pixel ratio */
	height = scr_height;
	width = av_rescale(height, aspect_ratio.num, aspect_ratio.den) & ~1;
	if (width > scr_width) {
		width = scr_width;
		height = av_rescale(width, aspect_ratio.den, aspect_ratio.num) & ~1;
	}
	x = (scr_width - width) / 2;
	y = (scr_height - height) / 2;
	rect->x = (int)(scr_xleft + x);
	rect->y = (int)(scr_ytop  + y);
	rect->w = FFMAX((int)width, 1);
	rect->h = FFMAX((int)height, 1);
}

void get_sdl_pix_fmt_and_blendmode(int format, Uint32 *sdl_pix_fmt, SDL_BlendMode *sdl_blendmode)
{
	int i;
	*sdl_blendmode = SDL_BLENDMODE_NONE;
	*sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;
	if (format == AV_PIX_FMT_RGB32 ||
		format == AV_PIX_FMT_RGB32_1 ||
		format == AV_PIX_FMT_BGR32 ||
		format == AV_PIX_FMT_BGR32_1)
		*sdl_blendmode = SDL_BLENDMODE_BLEND;
	for (i = 0; i < FF_ARRAY_ELEMS(sdl_texture_format_map) - 1; i++) {
		if (format == sdl_texture_format_map[i].format) {
			*sdl_pix_fmt = sdl_texture_format_map[i].texture_fmt;
			return;
		}
	}
}

int upload_texture(SDL_Texture **tex, AVFrame *frame, struct SwsContext **img_convert_ctx) {
	int ret = 0;
	Uint32 sdl_pix_fmt;
	SDL_BlendMode sdl_blendmode;
	get_sdl_pix_fmt_and_blendmode(frame->format, &sdl_pix_fmt, &sdl_blendmode);
	if (realloc_texture(tex, sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN ? SDL_PIXELFORMAT_ARGB8888 : sdl_pix_fmt, frame->width, frame->height, sdl_blendmode, 0) < 0)
		return -1;
	switch (sdl_pix_fmt) {
	case SDL_PIXELFORMAT_UNKNOWN:
		/* This should only happen if we are not using avfilter... */
		*img_convert_ctx = sws_getCachedContext(*img_convert_ctx, frame->width, frame->height, 
			(AVPixelFormat)frame->format, frame->width, frame->height,
			(AVPixelFormat)AV_PIX_FMT_BGRA, sws_flags, NULL, NULL, NULL);
		if (*img_convert_ctx != NULL) {
			uint8_t *pixels[4];
			int pitch[4];
			if (!SDL_LockTexture(*tex, NULL, (void **)pixels, pitch)) {
				sws_scale(*img_convert_ctx, (const uint8_t * const *)frame->data, frame->linesize,
					0, frame->height, pixels, pitch);
				SDL_UnlockTexture(*tex);
			}
		}
		else {
			dmsg("Cannot initialize the conversion context\n");
			ret = -1;
		}
		break;
	case SDL_PIXELFORMAT_IYUV:
		if (frame->linesize[0] > 0 && frame->linesize[1] > 0 && frame->linesize[2] > 0) {
			ret = SDL_UpdateYUVTexture(*tex, NULL, frame->data[0], frame->linesize[0],
				frame->data[1], frame->linesize[1],
				frame->data[2], frame->linesize[2]);
		}
		else if (frame->linesize[0] < 0 && frame->linesize[1] < 0 && frame->linesize[2] < 0) {
			ret = SDL_UpdateYUVTexture(*tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1), -frame->linesize[0],
				frame->data[1] + frame->linesize[1] * (AV_CEIL_RSHIFT(frame->height, 1) - 1), -frame->linesize[1],
				frame->data[2] + frame->linesize[2] * (AV_CEIL_RSHIFT(frame->height, 1) - 1), -frame->linesize[2]);
		}
		else {
			dmsg("Mixed negative and positive linesizes are not supported.\n");
			return -1;
		}
		break;
	default:
		if (frame->linesize[0] < 0) {
			ret = SDL_UpdateTexture(*tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1), -frame->linesize[0]);
		}
		else {
			ret = SDL_UpdateTexture(*tex, NULL, frame->data[0], frame->linesize[0]);
		}
		break;
	}
	return ret;
}

void set_sdl_yuv_conversion_mode(AVFrame *frame)
{
#if SDL_VERSION_ATLEAST(2,0,8)
	SDL_YUV_CONVERSION_MODE mode = SDL_YUV_CONVERSION_AUTOMATIC;
	if (frame && (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUYV422 || frame->format == AV_PIX_FMT_UYVY422)) {
		if (frame->color_range == AVCOL_RANGE_JPEG)
			mode = SDL_YUV_CONVERSION_JPEG;
		else if (frame->colorspace == AVCOL_SPC_BT709)
			mode = SDL_YUV_CONVERSION_BT709;
		else if (frame->colorspace == AVCOL_SPC_BT470BG || frame->colorspace == AVCOL_SPC_SMPTE170M || frame->colorspace == AVCOL_SPC_SMPTE240M)
			mode = SDL_YUV_CONVERSION_BT601;
	}
	SDL_SetYUVConversionMode(mode);
#endif
}

void video_image_display(VideoState *is)
{
	Frame *vp;
	Frame *sp = NULL;
	
	vp = frame_queue_peek_last(&is->pictq);	
#if 0
	if (is->subtitle_st)
	{
		if (frame_queue_nb_remaining(&is->subpq) > 0) {
			sp = frame_queue_peek(&is->subpq);

			if (vp->pts >= sp->pts + ((float)sp->sub.start_display_time / 1000)) {
				if (!sp->uploaded) {
					uint8_t* pixels[4];
					int pitch[4];
					int i;
					if (!sp->width || !sp->height) {
						sp->width = vp->width;
						sp->height = vp->height;
					}
					if (realloc_texture(&is->sub_texture, SDL_PIXELFORMAT_ARGB8888, sp->width, sp->height, SDL_BLENDMODE_BLEND, 1) < 0)
						return;

					for (i = 0; i < (int)sp->sub.num_rects; i++) {
						AVSubtitleRect *sub_rect = sp->sub.rects[i];

						sub_rect->x = av_clip(sub_rect->x, 0, sp->width);
						sub_rect->y = av_clip(sub_rect->y, 0, sp->height);
						sub_rect->w = av_clip(sub_rect->w, 0, sp->width - sub_rect->x);
						sub_rect->h = av_clip(sub_rect->h, 0, sp->height - sub_rect->y);

						is->sub_convert_ctx = sws_getCachedContext(is->sub_convert_ctx,
							sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8,
							sub_rect->w, sub_rect->h, AV_PIX_FMT_BGRA,
							0, NULL, NULL, NULL);
						if (!is->sub_convert_ctx) {
							dmsg("Cannot initialize the conversion context\n");
							return;
						}
						if (!SDL_LockTexture(is->sub_texture, (SDL_Rect *)sub_rect, (void **)pixels, pitch)) {
							sws_scale(is->sub_convert_ctx, (const uint8_t * const *)sub_rect->data, sub_rect->linesize,
								0, sub_rect->h, pixels, pitch);
							SDL_UnlockTexture(is->sub_texture);
						}
					}
					sp->uploaded = 1;
				}
			}
			else
				sp = NULL;
		}
	}
#endif
	if (is->imgw != vp->width) {
		is->imgw = vp->width;
	}
	if (is->imgh != vp->height) {
		is->imgh = vp->height;
	}
	if (is->vp_width != vp->width) {
		is->vp_width = vp->width;
	}
	if (is->vp_height != vp->height) {
		is->vp_height = vp->height;
	}
	if (is->vp_sar.den != vp->sar.den || is->vp_sar.num != vp->sar.num) {
		is->vp_sar.den = vp->sar.den;
		is->vp_sar.num = vp->sar.num;
	}
	calculate_display_rect(&is->rect, is->xleft, is->ytop, is->width, is->height, vp->width, vp->height, vp->sar);

	if (!vp->uploaded) {
		if (upload_texture(&is->vid_texture, vp->frame, &is->img_convert_ctx) < 0)
			return;
		vp->uploaded = 1;
		vp->flip_v = vp->frame->linesize[0] < 0;

		/*uint8_t *pixels = NULL;
		int pitch = 0, j = 0;
		SDL_Rect vid_rect = { 0, 0, m_is->width/2, m_is->height/2 };
		if (!SDL_LockTexture(m_is->vid_texture, (SDL_Rect *)&vid_rect, (void **)&pixels, &pitch)) {
			//for (j = 0; j < vid_rect.h / 10; j++, pixels += pitch)
			//	memset(pixels, 0, vid_rect.w << 2);
			SDL_UnlockTexture(m_is->vid_texture);
		}*/
	}

	set_sdl_yuv_conversion_mode(vp->frame);
	SDL_RenderCopyEx(m_sdlRend, is->vid_texture, NULL, &is->rect, 0, NULL, vp->flip_v ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);
	set_sdl_yuv_conversion_mode(NULL);
	if (sp) {
#if USE_ONEPASS_SUBTITLE_RENDER
		SDL_RenderCopy(m_sdlRend, is->sub_texture, NULL, &is->rect);
#else
		int i;
		double xratio = (double)rect.w / (double)sp->width;
		double yratio = (double)rect.h / (double)sp->height;
		for (i = 0; i < sp->sub.num_rects; i++) {
			SDL_Rect *sub_rect = (SDL_Rect*)sp->sub.rects[i];
			SDL_Rect target = { .x = rect.x + sub_rect->x * xratio,
							   .y = rect.y + sub_rect->y * yratio,
							   .w = sub_rect->w * xratio,
							   .h = sub_rect->h * yratio };
			SDL_RenderCopy(m_sdlRend, is->sub_texture, sub_rect, &target);
		}
#endif		
	}

	if (rendercallback && cb_receiver) {
		(*rendercallback)(cb_receiver, pbsdl_getposition());
	}
}

//-----------------------------------------------------------------------------
// stream
//-----------------------------------------------------------------------------
void video_audio_display(VideoState *s)
{
	int i, i_start, x, y1, y, ys, delay, n, nb_display_channels;
	int ch, channels, h, h2;
	int64_t time_diff;
	int rdft_bits, nb_freq;

	for (rdft_bits = 1; (1 << rdft_bits) < 2 * s->height; rdft_bits++)
		;
	nb_freq = 1 << (rdft_bits - 1);

	/* compute display index : center on currently output samples */
	channels = s->audio_tgt.channels;
	nb_display_channels = channels;
	if (!s->paused) {
		int data_used = s->show_mode == SHOW_MODE_WAVES ? s->width : (2 * nb_freq);
		n = 2 * channels;
		delay = s->audio_write_buf_size;
		delay /= n;

		/* to be more precise, we take into account the time spent since
		   the last buffer computation */
		if (audio_callback_time) {
			time_diff = av_gettime_relative() - audio_callback_time;
			delay -= (int)((time_diff * s->audio_tgt.freq) / 1000000);
		}

		delay += 2 * data_used;
		if (delay < data_used)
			delay = data_used;

		i_start = x = compute_mod(s->sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);
		if (s->show_mode == SHOW_MODE_WAVES) {
			h = INT_MIN;
			for (i = 0; i < 1000; i += channels) {
				int idx = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
				int a = s->sample_array[idx];
				int b = s->sample_array[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
				int c = s->sample_array[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
				int d = s->sample_array[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
				int score = a - d;
				if (h < score && (b ^ c) < 0) {
					h = score;
					i_start = idx;
				}
			}
		}

		s->last_i_start = i_start;
	}
	else {
		i_start = s->last_i_start;
	}

	if (s->show_mode == SHOW_MODE_WAVES) {
		SDL_SetRenderDrawColor(m_sdlRend, 255, 255, 255, 255);

		/* total height for one channel */
		h = s->height / nb_display_channels;
		/* graph height / 2 */
		h2 = (h * 9) / 20;
		for (ch = 0; ch < nb_display_channels; ch++) {
			i = i_start + ch;
			y1 = s->ytop + ch * h + (h / 2); /* position of center line */
			for (x = 0; x < s->width; x++) {
				y = (s->sample_array[i] * h2) >> 15;
				if (y < 0) {
					y = -y;
					ys = y1 - y;
				}
				else {
					ys = y1;
				}
				fill_rectangle(s->xleft + x, ys, 1, y);
				i += channels;
				if (i >= SAMPLE_ARRAY_SIZE)
					i -= SAMPLE_ARRAY_SIZE;
			}
		}

		SDL_SetRenderDrawColor(m_sdlRend, 0, 0, 255, 255);

		for (ch = 1; ch < nb_display_channels; ch++) {
			y = s->ytop + ch * h;
			fill_rectangle(s->xleft, y, s->width, 1);
		}
	}
	else {
		#if 0
		if (realloc_texture(&s->vis_texture, SDL_PIXELFORMAT_ARGB8888, s->width, s->height, SDL_BLENDMODE_NONE, 1) < 0)
			return;

		nb_display_channels = FFMIN(nb_display_channels, 2);
		if (rdft_bits != s->rdft_bits) {
			av_rdft_end(s->rdft);
			av_free(s->rdft_data);
			s->rdft = av_rdft_init(rdft_bits, DFT_R2C);
			s->rdft_bits = rdft_bits;
			s->rdft_data = av_malloc_array(nb_freq, 4 * sizeof(*s->rdft_data));
		}
		if (!s->rdft || !s->rdft_data) {
			dmsg("Failed to allocate buffers for RDFT, switching to waves display\n");
			s->show_mode = SHOW_MODE_WAVES;
		}
		else {
			FFTSample *data[2];
			SDL_Rect rect = { .x = s->xpos,.y = 0,.w = 1,.h = s->height };
			uint32_t *pixels;
			int pitch;
			for (ch = 0; ch < nb_display_channels; ch++) {
				data[ch] = s->rdft_data + 2 * nb_freq * ch;
				i = i_start + ch;
				for (x = 0; x < 2 * nb_freq; x++) {
					double w = (x - nb_freq) * (1.0 / nb_freq);
					data[ch][x] = s->sample_array[i] * (1.0 - w * w);
					i += channels;
					if (i >= SAMPLE_ARRAY_SIZE)
						i -= SAMPLE_ARRAY_SIZE;
				}
				av_rdft_calc(s->rdft, data[ch]);
			}
			/* Least efficient way to do this, we should of course
			 * directly access it but it is more than fast enough. */
			if (!SDL_LockTexture(s->vis_texture, &rect, (void **)&pixels, &pitch)) {
				pitch >>= 2;
				pixels += pitch * s->height;
				for (y = 0; y < s->height; y++) {
					double w = 1 / sqrt(nb_freq);
					int a = sqrt(w * sqrt(data[0][2 * y + 0] * data[0][2 * y + 0] + data[0][2 * y + 1] * data[0][2 * y + 1]));
					int b = (nb_display_channels == 2) ? sqrt(w * hypot(data[1][2 * y + 0], data[1][2 * y + 1]))
						: a;
					a = FFMIN(a, 255);
					b = FFMIN(b, 255);
					pixels -= pitch;
					*pixels = (a << 16) + (b << 8) + ((a + b) >> 1);
				}
				SDL_UnlockTexture(s->vis_texture);
			}
			SDL_RenderCopy(m_sdlRend, s->vis_texture, NULL, NULL);
		}
		if (!s->paused)
			s->xpos++;
		if (s->xpos >= s->width)
			s->xpos = s->xleft;
		#endif
	}
}

void stream_component_close(VideoState *is, int stream_index)
{
	AVFormatContext *ic = is->ic;
	AVCodecParameters *codecpar;

	if (stream_index < 0 || stream_index >= (int)ic->nb_streams)
		return;
	codecpar = ic->streams[stream_index]->codecpar;

	switch (codecpar->codec_type) {
	case AVMEDIA_TYPE_AUDIO:
		decoder_abort(&is->auddec, &is->sampq);
		SDL_CloseAudioDevice(audio_dev);
		decoder_destroy(&is->auddec);
		swr_free(&is->swr_ctx);
		av_freep(&is->audio_buf1);
		is->audio_buf1_size = 0;
		is->audio_buf = NULL;
		break;
	case AVMEDIA_TYPE_VIDEO:
		decoder_abort(&is->viddec, &is->pictq);
		decoder_destroy(&is->viddec);
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		decoder_abort(&is->subdec, &is->subpq);
		decoder_destroy(&is->subdec);
		break;
	default:
		break;
	}

	ic->streams[stream_index]->discard = AVDISCARD_ALL;
	switch (codecpar->codec_type) {
	case AVMEDIA_TYPE_AUDIO:
		is->audio_st = NULL;
		is->audio_stream = -1;
		break;
	case AVMEDIA_TYPE_VIDEO:
		is->video_st = NULL;
		is->video_stream = -1;
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		is->subtitle_st = NULL;
		is->subtitle_stream = -1;
		break;
	default:
		break;
	}
}

void stream_close(VideoState *is)
{
	/* XXX: use a special url_shutdown call to abort parse cleanly */
	is->abort_request = 1;
	SDL_WaitThread(is->read_tid, NULL);
	SDL_WaitThread(is->event_tid, NULL);
	SDL_WaitThread(is->monitor_tid, NULL);

	/* close each stream */
	if (is->audio_stream >= 0)
		stream_component_close(is, is->audio_stream);
	if (is->video_stream >= 0)
		stream_component_close(is, is->video_stream);
	if (is->subtitle_stream >= 0)
		stream_component_close(is, is->subtitle_stream);

	avformat_close_input(&is->ic);

	packet_queue_destroy(&is->videoq);
	packet_queue_destroy(&is->audioq);
	packet_queue_destroy(&is->subtitleq);

	/* free all pictures */
	frame_queue_destory(&is->pictq);
	frame_queue_destory(&is->sampq);
	frame_queue_destory(&is->subpq);
	SDL_DestroyCond(is->continue_read_thread);
	sws_freeContext(is->img_convert_ctx);
	sws_freeContext(is->sub_convert_ctx);
	av_free(is->filename);
	if (is->vis_texture)
		SDL_DestroyTexture(is->vis_texture);
	if (is->vid_texture)
		SDL_DestroyTexture(is->vid_texture);
	if (is->sub_texture)
		SDL_DestroyTexture(is->sub_texture);
	av_free(is);
}

void do_exit(VideoState *is)
{
	if (is) {
		stream_close(is);
	}
	if (m_is) {
		m_is = NULL;
	}
	if (m_sdlRend) {

		SDL_SetRenderDrawColor(m_sdlRend, 0, 0, 0, 255);
		SDL_RenderClear(m_sdlRend);		
		SDL_RenderPresent(m_sdlRend);

		SDL_DestroyRenderer(m_sdlRend);
		m_sdlRend = NULL;
	}
	if (m_sdlWnd) {
		SDL_DestroyWindow(m_sdlWnd);
		m_sdlWnd = NULL;
	}
	av_dict_free(&format_opts);
	avformat_network_deinit();
	SDL_Quit();
}

void set_default_window_size(int width, int height, AVRational sar)
{
	SDL_Rect rect;
	int max_width = screen_width ? screen_width : INT_MAX;
	int max_height = screen_height ? screen_height : INT_MAX;
	if (max_width == INT_MAX && max_height == INT_MAX)
		max_height = height;
	calculate_display_rect(&rect, 0, 0, max_width, max_height, width, height, sar);
	default_width = rect.w;
	default_height = rect.h;
}

int video_open(VideoState *is)
{
	int w, h;

	w = screen_width ? screen_width : default_width;
	h = screen_height ? screen_height : default_height;

	SDL_SetWindowTitle(m_sdlWnd, is->filename);

	SDL_SetWindowSize(m_sdlWnd, w, h);
#ifdef _DEBUG
	screen_left = 40;
	screen_top = 480;
#endif
	SDL_SetWindowPosition(m_sdlWnd, screen_left, screen_top);
	if (is_full_screen)
		SDL_SetWindowFullscreen(m_sdlWnd, SDL_WINDOW_FULLSCREEN_DESKTOP);
	SDL_ShowWindow(m_sdlWnd);

	is->width = w;
	is->height = h;

	return 0;
}

void draw_labels()
{
	SDL_Rect rt = { 0, 0, 300, 300 };
	sdl2_rectangle(rt, 0x80, 0xFF, 0xFF, 0xFF, true);


	SDL_Rect rt2 = { 300, 300, 100, 100 };
	sdl2_rectangle(rt2, 0x80, 0xFF, 0x00, 0x00, true);

	SDL_RenderDrawLine(m_sdlRend, 200, 200, 300, 300);
#if 1
	std::list<POINT> lst;
	POINT pt = { 100,200 };
	for (INT i = 0; i < 10; i++) {
		pt.x += 10;
		pt.y += 10;
		lst.push_back(pt);
	}

	SDL_SetRenderDrawColor(m_sdlRend, 0, 0xff, 0, 0xFF);

	list<POINT>::iterator itr = lst.begin();
	POINT ptp = *itr;
	for (; itr != lst.end(); itr++) {
		pt = *itr;
		SDL_Rect rt = { pt.x - 4, pt.y - 4, 8, 8 };
		//SDL_RenderSetScale(m_sdlRend, 1, 1);
		SDL_RenderFillRect(m_sdlRend, &rt);
		//SDL_RenderSetScale(m_sdlRend, 5, 5);
		SDL_RenderDrawLine(m_sdlRend, ptp.x / 5, ptp.y / 5, pt.x / 5, pt.y / 5);
		ptp = *itr;
	}
	lst.clear();
#endif
	// std:list   (elements at non contiguous memory, insertion and deletion, linked list)
	// std:vector (elements at contiguous memory, random access by index, array)
	/*std::vector<Point> vertices;
	vertices.push_back(Point(10, 10));
	vertices.push_back(Point(100, 10));
	vertices.push_back(Point(100, 200));
	vertices.push_back(Point(50, 50));
	vertices.push_back(Point(10, 200));
	vertices.clear();*/
		
	//SDL_Texture *texTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
	//	SDL_TEXTUREACCESS_TARGET, WIN_WIDTH, WIN_HEIGHT);
		
}

/* display the current picture, if any */
void video_display(VideoState *is)
{
	Frame* vp = frame_queue_peek_last(&is->pictq);
	//dmsg("display: %d\n", (int)(vp->frame->pts / 100));
	if (is->step == 1 && mTargetFrame > 0) {
		int icurfrm = pbsdl_getcurframe();
		//dmsg("curfrm.%d tarfrm.%d\n", icurfrm, mTargetFrame);
		if (icurfrm >= mTargetFrame) {
			mTargetFrame = 0;
			stream_toggle_pause(is);
		}
		else {
			double fps = (double)m_is->video_st->avg_frame_rate.num / (double)m_is->video_st->avg_frame_rate.den;
			set_clock(&is->extclk, (double)(mTargetFrame - 2) / fps, 0);
			//dmsg("set_clock: %f\n", (double)mTargetFrame / fps);
			return;
		}
	}

	if (!is->width)
		video_open(is);

	SDL_SetRenderDrawColor(m_sdlRend, 0, 0, 0, 255);
	SDL_RenderClear(m_sdlRend);
	if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO)
		video_audio_display(is);
	else if (is->video_st)
		video_image_display(is);

	//draw_labels();
	
	SDL_RenderPresent(m_sdlRend);
	
	/*SDL_LockSurface(m_sdlSurf);
	unsigned char* pMem = (unsigned char*)m_sdlSurf->pixels;
	int bytepx = m_sdlSurf->format->BytesPerPixel;
	int ipitch = m_sdlSurf->pitch;
	SDL_UnlockSurface(m_sdlSurf);
	*/
}

double get_clock(Clock *c)
{
	if (*c->queue_serial != c->serial)
		return NAN;
	if (c->paused) {
		return c->pts;
	}
	else {
		double time = av_gettime_relative() / 1000000.0;
		return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
	}
}

void set_clock_at(Clock *c, double pts, int serial, double time)
{
	c->pts = pts;
	c->last_updated = time;
	c->pts_drift = c->pts - time;
	c->serial = serial;
}

void set_clock(Clock *c, double pts, int serial)
{
	double time = av_gettime_relative() / 1000000.0;
	set_clock_at(c, pts, serial, time);
}

void set_clock_speed(Clock *c, double speed)
{
	set_clock(c, get_clock(c), c->serial);
	c->speed = speed;
}

void init_clock(Clock *c, int *queue_serial)
{
	c->speed = 1.0;
	c->paused = 0;
	c->queue_serial = queue_serial;
	set_clock(c, NAN, -1);
}

void sync_clock_to_slave(Clock *c, Clock *slave)
{
	double clock = get_clock(c);
	double slave_clock = get_clock(slave);
	if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
		set_clock(c, slave_clock, slave->serial);
}

int get_master_sync_type(VideoState *is)
{
	if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
		if (is->video_st)
			return AV_SYNC_VIDEO_MASTER;
		else
			return AV_SYNC_AUDIO_MASTER;
	}
	else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
		if (is->audio_st)
			return AV_SYNC_AUDIO_MASTER;
		else
			return AV_SYNC_EXTERNAL_CLOCK;
	}
	else {
		return AV_SYNC_EXTERNAL_CLOCK;
	}
}

/* get the current master clock value */
double get_master_clock(VideoState *is)
{
	double val;

	switch (get_master_sync_type(is)) {
	case AV_SYNC_VIDEO_MASTER:
		val = get_clock(&is->vidclk);
		break;
	case AV_SYNC_AUDIO_MASTER:
		val = get_clock(&is->audclk);
		break;
	default:
		val = get_clock(&is->extclk);
		break;
	}
	return val;
}

void check_external_clock_speed(VideoState *is)
{
	if (is->video_stream >= 0 && is->videoq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES ||
		is->audio_stream >= 0 && is->audioq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES) {
		set_clock_speed(&is->extclk, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, is->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
	}
	else if ((is->video_stream < 0 || is->videoq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
		(is->audio_stream < 0 || is->audioq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES)) {
		set_clock_speed(&is->extclk, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, is->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
	}
	else {
		double speed = is->extclk.speed;
		if (speed != 1.0)
			set_clock_speed(&is->extclk, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
	}
}

void stream_seek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes)
{
	if (!is->seek_req) {
		is->seek_pos = pos;
		is->seek_rel = rel;
		is->seek_flags &= ~AVSEEK_FLAG_BYTE;
		if (seek_by_bytes)
			is->seek_flags |= AVSEEK_FLAG_BYTE;
		is->seek_req = 1;
		SDL_CondSignal(is->continue_read_thread);
	}
}

/* pause or resume the video */
void stream_toggle_pause(VideoState *is)
{
	if (is->paused) {
		is->frame_timer += av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
		if (is->read_pause_return != AVERROR(ENOSYS)) {
			is->vidclk.paused = 0;
		}
		set_clock(&is->vidclk, get_clock(&is->vidclk), is->vidclk.serial);
	}
	set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
	is->paused = is->audclk.paused = is->vidclk.paused = is->extclk.paused = !is->paused;
}

void toggle_pause(VideoState *is)
{
	stream_toggle_pause(is);
	is->step = 0;
}

void toggle_mute(VideoState *is)
{
	is->muted = !is->muted;
}

bool pbsdl_ispaused()
{
	if (m_is) {
		if (!m_is->paused) {
			return FALSE;
		}
	}
	return TRUE;
}

void pbsdl_seekto(double dPos)
{
	if (m_is) {
		if (m_is->ic) {
			if (m_is->ic->start_time != AV_NOPTS_VALUE && dPos < m_is->ic->start_time / (double)AV_TIME_BASE) {
				dPos = m_is->ic->start_time / (double)AV_TIME_BASE;
			}
			stream_seek(m_is, (int64_t)(dPos * AV_TIME_BASE), (int64_t)(1 * AV_TIME_BASE), 0);
		}
	}
}

void pbsdl_stop()
{
	if (m_is) {
		if (!m_is->paused) {
			stream_toggle_pause(m_is);
		}
		stream_seek(m_is, 0, 0, 0);
	}
}

void pbsdl_pause()
{
	if (m_is) {
		m_is->user_paused = 1;
		if (!m_is->paused) {
			stream_toggle_pause(m_is);
			m_is->step = 0;
		}
	}
}

void pbsdl_play()
{
	if (m_is) {
		m_is->user_paused = 0;
		if (m_is->paused) {
			if (m_is->realtime && g_jbuf_enable && !m_is->prebuffering) {
				// Realtime stream resuming from a user pause: the buffered
				// packets are stale (queued while paused, now behind live).
				// Don't play through that backlog — flush it and re-buffer
				// from live instead. Stays paused until read_thread reports
				// the fresh buffer is ready (FF_JBUF_READY_EVENT), so we
				// never show old, lagged frames after resume.
				m_is->resync_req = 1;
				SDL_CondSignal(m_is->continue_read_thread);
			}
			else if (!m_is->prebuffering) {
				stream_toggle_pause(m_is);
				m_is->step = 0;
			}
		}
	}
}

void pbsdl_stepfw()
{
	if (m_is) {
		if (m_is->paused)
			stream_toggle_pause(m_is);
		m_is->step = 1;
	}
}

void pbsdl_jumpTo(int iFrames)
{
	//mJumpTo = iFrames;
	//mSkipFrame = iFrames;
	if (m_is) {		
		if (iFrames >= m_is->video_st->nb_frames) {
			iFrames = (int)(m_is->video_st->nb_frames - 1);
		}
		mTargetFrame = iFrames;
	}
	

	/*double fps = (double)m_is->video_st->avg_frame_rate.num / (double)m_is->video_st->avg_frame_rate.den;
	double seek_target = mTargetFrame * fps;
	set_clock(&m_is->extclk, seek_target, 0);*/

	pbsdl_stepfw();
}

int pbsdl_isMute()
{
	if (m_is) {
		return (int)m_is->muted;
	}
	return 1;
}

void pbsdl_mute(int iMute)
{
	if (m_is) {
		m_is->muted = iMute;
	}
}

void pbsdl_volume(int vol)
{
	if (m_is) {
		double volume_level = vol ? (20 * log(vol / (double)SDL_MIX_MAXVOLUME) / log(10)) : -1000.0;
		int new_volume = lrint(SDL_MIX_MAXVOLUME * pow(10.0, (volume_level) / 20.0));
		m_is->audio_volume = av_clip(m_is->audio_volume == new_volume ? (m_is->audio_volume) : new_volume, 0, SDL_MIX_MAXVOLUME);
	}
}

void update_volume(VideoState *is, int sign, double step)
{
	double volume_level = is->audio_volume ? (20 * log(is->audio_volume / (double)SDL_MIX_MAXVOLUME) / log(10)) : -1000.0;
	int new_volume = lrint(SDL_MIX_MAXVOLUME * pow(10.0, (volume_level + sign * step) / 20.0));
	is->audio_volume = av_clip(is->audio_volume == new_volume ? (is->audio_volume + sign) : new_volume, 0, SDL_MIX_MAXVOLUME);
}

void step_to_next_frame(VideoState *is)
{
	if (is->paused)
		stream_toggle_pause(is);
	is->step = 1;
}

double compute_target_delay(double delay, VideoState *is)
{
	double sync_threshold, diff = 0;

	/* update delay to follow master synchronisation source */
	if (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER) {
		/* if video is slave, we try to correct big delays by
		   duplicating or deleting a frame */
		diff = get_clock(&is->vidclk) - get_master_clock(is);

		/* skip or repeat frame. We take into account the
		   delay to compute the threshold. I still don't know
		   if it is the best guess */
		sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
		if (!isnan(diff) && fabs(diff) < is->max_frame_duration) {
			if (diff <= -sync_threshold)
				delay = FFMAX(0, delay + diff);
			else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
				delay = delay + diff;
			else if (diff >= sync_threshold)
				delay = 2 * delay;
		}
	}

	//dmsg("video: delay=%0.3f A-V=%f\n", delay, -diff);

	return delay;
}

double vp_duration(VideoState *is, Frame *vp, Frame *nextvp) {
	if (vp->serial == nextvp->serial) {
		double duration = nextvp->pts - vp->pts;
		if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration)
			return vp->duration;
		else
			return duration;
	}
	else {
		return 0.0;
	}
}

void update_video_pts(VideoState *is, double pts, int64_t pos, int serial) {
	/* update current video pts */
	set_clock(&is->vidclk, pts, serial);
	sync_clock_to_slave(&is->extclk, &is->vidclk);
}

/* called to display each frame */
void video_refresh(void *opaque, double *remaining_time)
{
	VideoState *is = (VideoState*)opaque;
	double time;

	Frame *sp, *sp2;

	if (!is->paused && get_master_sync_type(is) == AV_SYNC_EXTERNAL_CLOCK && is->realtime)
		check_external_clock_speed(is);

	if (!display_disable && is->show_mode != SHOW_MODE_VIDEO && is->audio_st) {
		time = av_gettime_relative() / 1000000.0;
		if (is->force_refresh || is->last_vis_time + rdftspeed < time) {
			video_display(is);
			is->last_vis_time = time;
		}
		*remaining_time = FFMIN(*remaining_time, is->last_vis_time + rdftspeed - time);
	}

	if (is->video_st) {
	retry:
		if (frame_queue_nb_remaining(&is->pictq) == 0) {
			// nothing to do, no picture to display in the queue
		}
		else {
			double last_duration, duration, delay;
			Frame *vp, *lastvp;

			/* dequeue the picture */
			lastvp = frame_queue_peek_last(&is->pictq);
			vp = frame_queue_peek(&is->pictq);

			if (vp->serial != is->videoq.serial) {
				frame_queue_next(&is->pictq);
				goto retry;
			}

			if (lastvp->serial != vp->serial)
				is->frame_timer = av_gettime_relative() / 1000000.0;

			if (is->paused)
				goto display;

			/* compute nominal last_duration */
			last_duration = vp_duration(is, lastvp, vp);
			delay = compute_target_delay(last_duration, is);

			time = av_gettime_relative() / 1000000.0;
			if (time < is->frame_timer + delay) {
				*remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
				goto display;
			}

			is->frame_timer += delay;
			if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX)
				is->frame_timer = time;

			SDL_LockMutex(is->pictq.mutex);
			if (!isnan(vp->pts))
				update_video_pts(is, vp->pts, vp->pos, vp->serial);
			SDL_UnlockMutex(is->pictq.mutex);

			if (frame_queue_nb_remaining(&is->pictq) > 1) {
				Frame *nextvp = frame_queue_peek_next(&is->pictq);
				duration = vp_duration(is, vp, nextvp);
				if (!is->step && (framedrop > 0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) && time > is->frame_timer + duration) {
					is->frame_drops_late++;
					frame_queue_next(&is->pictq);
					goto retry;
				}
			}

			if (is->subtitle_st) {
				while (frame_queue_nb_remaining(&is->subpq) > 0) {
					sp = frame_queue_peek(&is->subpq);

					if (frame_queue_nb_remaining(&is->subpq) > 1)
						sp2 = frame_queue_peek_next(&is->subpq);
					else
						sp2 = NULL;

					if (sp->serial != is->subtitleq.serial
						|| (is->vidclk.pts > (sp->pts + ((float)sp->sub.end_display_time / 1000)))
						|| (sp2 && is->vidclk.pts > (sp2->pts + ((float)sp2->sub.start_display_time / 1000))))
					{
						if (sp->uploaded) {
							int i;
							for (i = 0; i < (int)sp->sub.num_rects; i++) {
								AVSubtitleRect *sub_rect = sp->sub.rects[i];
								uint8_t *pixels;
								int pitch, j;

								if (!SDL_LockTexture(is->sub_texture, (SDL_Rect *)sub_rect, (void **)&pixels, &pitch)) {
									for (j = 0; j < sub_rect->h; j++, pixels += pitch)
										memset(pixels, 0, sub_rect->w << 2);
									SDL_UnlockTexture(is->sub_texture);
								}
							}
						}
						frame_queue_next(&is->subpq);
					}
					else {
						break;
					}
				}
			}
#if 0
			if (mTargetFrame > 0) {
				while (mTargetFrame > 0) {
					frame_queue_next(&is->pictq);
					/*Frame* vp = frame_queue_peek_last(&is->pictq);
					if (vp) {
						if (vp->pts) {
							vp->pts >
						}
					}*/
					int icurfrm = pbsdl_getcurframe();
					if (icurfrm >= mTargetFrame) {
						mTargetFrame = 0;
					}
				}
			}
			else {
				frame_queue_next(&is->pictq);
			}
#else
			frame_queue_next(&is->pictq);
#endif
			is->force_refresh = 1;

			if (is->step && !is->paused) {
#if 1
				if (is->step && mTargetFrame > 0) {
					/*int icurfrm = pbsdl_getcurframe();
					dmsg("curfrm.%d tarfrm.%d\n", icurfrm, mTargetFrame);
					if (icurfrm >= mTargetFrame) {
						mTargetFrame = 0;
						stream_toggle_pause(is);
					}*/
				}
				else {
					stream_toggle_pause(is);
				}
#else			
				stream_toggle_pause(is);
#endif
			}
		}
	display:
		/* display picture */
		if (!display_disable && is->force_refresh && is->show_mode == SHOW_MODE_VIDEO && is->pictq.rindex_shown)
			video_display(is);
	}
	is->force_refresh = 0;
	if (show_status) {
		#if 0
		AVBPrint buf;
		static int64_t last_time;
		int64_t cur_time;
		int aqsize, vqsize, sqsize;
		double av_diff;

		cur_time = av_gettime_relative();
		if (!last_time || (cur_time - last_time) >= 30000) {
			aqsize = 0;
			vqsize = 0;
			sqsize = 0;
			if (is->audio_st)
				aqsize = is->audioq.size;
			if (is->video_st)
				vqsize = is->videoq.size;
			if (is->subtitle_st)
				sqsize = is->subtitleq.size;
			av_diff = 0;
			if (is->audio_st && is->video_st)
				av_diff = get_clock(&is->audclk) - get_clock(&is->vidclk);
			else if (is->video_st)
				av_diff = get_master_clock(is) - get_clock(&is->vidclk);
			else if (is->audio_st)
				av_diff = get_master_clock(is) - get_clock(&is->audclk);

			av_bprint_init(&buf, 0, AV_BPRINT_SIZE_AUTOMATIC);
			av_bprintf(&buf,
				"%7.2f %s:%7.3f fd=%4d aq=%5dKB vq=%5dKB sq=%5dB f=%"PRId64"/%"PRId64"   \r",
				get_master_clock(is),
				(is->audio_st && is->video_st) ? "A-V" : (is->video_st ? "M-V" : (is->audio_st ? "M-A" : "   ")),
				av_diff,
				is->frame_drops_early + is->frame_drops_late,
				aqsize / 1024,
				vqsize / 1024,
				sqsize,
				is->video_st ? is->viddec.avctx->pts_correction_num_faulty_dts : 0,
				is->video_st ? is->viddec.avctx->pts_correction_num_faulty_pts : 0);

			if (show_status == 1 && AV_LOG_INFO > av_log_get_level())
				fprintf(stderr, "%s", buf.str);
			else
				dmsg("%s", buf.str);

			fflush(stderr);
			av_bprint_finalize(&buf, NULL);

			last_time = cur_time;
		}
	#endif
	}
}

int queue_picture(VideoState *is, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial)
{
	Frame *vp;

#if defined(DEBUG_SYNC)
	printf("frame_type=%c pts=%0.3f\n",
		av_get_picture_type_char(src_frame->pict_type), pts);
#endif

	if (!(vp = frame_queue_peek_writable(&is->pictq)))
		return -1;

	vp->sar = src_frame->sample_aspect_ratio;
	vp->uploaded = 0;
		
	vp->width = src_frame->width;
	vp->height = src_frame->height;
	vp->format = src_frame->format;

	vp->pts = pts;
	vp->duration = duration;
	vp->pos = pos;
	vp->serial = serial;

	set_default_window_size(vp->width, vp->height, vp->sar);

	av_frame_move_ref(vp->frame, src_frame);
	frame_queue_push(&is->pictq);
	return 0;
}

int get_video_frame(VideoState *is, AVFrame *frame)
{
	int got_picture;

	if ((got_picture = decoder_decode_frame(&is->viddec, frame, NULL)) < 0)
		return -1;

	if (got_picture) {
		double dpts = NAN;

		if (frame->pts != AV_NOPTS_VALUE)
			dpts = av_q2d(is->video_st->time_base) * frame->pts;

		frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);

		if (framedrop > 0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) {
			if (frame->pts != AV_NOPTS_VALUE) {
				double diff = dpts - get_master_clock(is);
				if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
					diff - is->frame_last_filter_delay < 0 &&
					is->viddec.pkt_serial == is->vidclk.serial &&
					is->videoq.nb_packets) {
					is->frame_drops_early++;
					av_frame_unref(frame);
					got_picture = 0;
				}
			}
		}
	}

	return got_picture;
}

int audio_thread(void *arg)
{
	VideoState *is = (VideoState *)arg;
	AVFrame *frame = av_frame_alloc();
	Frame *af;
#if CONFIG_AVFILTER
	int last_serial = -1;
	int64_t dec_channel_layout;
	int reconfigure;
#endif
	int got_frame = 0;
	AVRational tb;
	int ret = 0;

	if (!frame)
		return AVERROR(ENOMEM);

	do {
		if ((got_frame = decoder_decode_frame(&is->auddec, frame, NULL)) < 0)
			goto the_end;

		if (got_frame) {
			tb.num = 1;
			tb.den = frame->sample_rate;

#if CONFIG_AVFILTER
			dec_channel_layout = get_valid_channel_layout(frame->channel_layout, frame->channels);

			reconfigure =
				cmp_audio_fmts(is->audio_filter_src.fmt, is->audio_filter_src.channels,
					frame->format, frame->channels) ||
				is->audio_filter_src.channel_layout != dec_channel_layout ||
				is->audio_filter_src.freq != frame->sample_rate ||
				is->auddec.pkt_serial != last_serial;

			if (reconfigure) {
				char buf1[1024], buf2[1024];
				av_get_channel_layout_string(buf1, sizeof(buf1), -1, is->audio_filter_src.channel_layout);
				av_get_channel_layout_string(buf2, sizeof(buf2), -1, dec_channel_layout);
				av_log(NULL, AV_LOG_DEBUG,
					"Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d fmt:%s layout:%s serial:%d\n",
					is->audio_filter_src.freq, is->audio_filter_src.channels, av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1, last_serial,
					frame->sample_rate, frame->channels, av_get_sample_fmt_name(frame->format), buf2, is->auddec.pkt_serial);

				is->audio_filter_src.fmt = frame->format;
				is->audio_filter_src.channels = frame->channels;
				is->audio_filter_src.channel_layout = dec_channel_layout;
				is->audio_filter_src.freq = frame->sample_rate;
				last_serial = is->auddec.pkt_serial;

				if ((ret = configure_audio_filters(is, afilters, 1)) < 0)
					goto the_end;
			}

			if ((ret = av_buffersrc_add_frame(is->in_audio_filter, frame)) < 0)
				goto the_end;

			while ((ret = av_buffersink_get_frame_flags(is->out_audio_filter, frame, 0)) >= 0) {
				tb = av_buffersink_get_time_base(is->out_audio_filter);
#endif
				if (!(af = frame_queue_peek_writable(&is->sampq)))
					goto the_end;

				af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
				af->pos = frame->pkt_pos;
				af->serial = is->auddec.pkt_serial;
				AVRational avr = { frame->nb_samples, frame->sample_rate };
				af->duration = av_q2d(avr);
				av_frame_move_ref(af->frame, frame);
				frame_queue_push(&is->sampq);

#if CONFIG_AVFILTER
				if (is->audioq.serial != is->auddec.pkt_serial)
					break;
			}
			if (ret == AVERROR_EOF)
				is->auddec.finished = is->auddec.pkt_serial;
#endif
		}
	} while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
the_end:
#if CONFIG_AVFILTER
	avfilter_graph_free(&is->agraph);
#endif
	av_frame_free(&frame);
	return ret;
}

int decoder_start(Decoder *d, int(*fn)(void *), const char *thread_name, void* arg)
{
	packet_queue_start(d->queue);
	d->decoder_tid = SDL_CreateThread(fn, thread_name, arg);
	if (!d->decoder_tid) {
		dmsg("SDL_CreateThread(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
	return 0;
}

int video_thread(void *arg)
{
	VideoState *is = (VideoState *)arg;
	AVFrame *frame = av_frame_alloc();
	double pts;
	double duration;
	int ret;
	AVRational tb = is->video_st->time_base;
	AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);

#if CONFIG_AVFILTER
	AVFilterGraph *graph = NULL;
	AVFilterContext *filt_out = NULL, *filt_in = NULL;
	int last_w = 0;
	int last_h = 0;
	enum AVPixelFormat last_format = -2;
	int last_serial = -1;
	int last_vfilter_idx = 0;
#endif

	if (!frame)
		return AVERROR(ENOMEM);

	for (;;) {
		ret = get_video_frame(is, frame);
		if (ret < 0)
			goto the_end;
		if (!ret)
			continue;

		if (mJumpTo > 0) {
			mJumpTo--;
			continue;
		}

#if CONFIG_AVFILTER
		if (last_w != frame->width
			|| last_h != frame->height
			|| last_format != frame->format
			|| last_serial != is->viddec.pkt_serial
			|| last_vfilter_idx != is->vfilter_idx) {
			av_log(NULL, AV_LOG_DEBUG,
				"Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s serial:%d\n",
				last_w, last_h,
				(const char *)av_x_if_null(av_get_pix_fmt_name(last_format), "none"), last_serial,
				frame->width, frame->height,
				(const char *)av_x_if_null(av_get_pix_fmt_name(frame->format), "none"), is->viddec.pkt_serial);
			avfilter_graph_free(&graph);
			graph = avfilter_graph_alloc();
			if (!graph) {
				ret = AVERROR(ENOMEM);
				goto the_end;
			}
			graph->nb_threads = filter_nbthreads;
			if ((ret = configure_video_filters(graph, is, vfilters_list ? vfilters_list[is->vfilter_idx] : NULL, frame)) < 0) {
				SDL_Event event;
				event.type = FF_QUIT_EVENT;
				event.user.data1 = is;
				SDL_PushEvent(&event);
				goto the_end;
			}
			filt_in = is->in_video_filter;
			filt_out = is->out_video_filter;
			last_w = frame->width;
			last_h = frame->height;
			last_format = frame->format;
			last_serial = is->viddec.pkt_serial;
			last_vfilter_idx = is->vfilter_idx;
			frame_rate = av_buffersink_get_frame_rate(filt_out);
		}

		ret = av_buffersrc_add_frame(filt_in, frame);
		if (ret < 0)
			goto the_end;

		while (ret >= 0) {
			is->frame_last_returned_time = av_gettime_relative() / 1000000.0;

			ret = av_buffersink_get_frame_flags(filt_out, frame, 0);
			if (ret < 0) {
				if (ret == AVERROR_EOF)
					is->viddec.finished = is->viddec.pkt_serial;
				ret = 0;
				break;
			}

			is->frame_last_filter_delay = av_gettime_relative() / 1000000.0 - is->frame_last_returned_time;
			if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
				is->frame_last_filter_delay = 0;
			tb = av_buffersink_get_time_base(filt_out);
#endif
			AVRational avr = { frame_rate.den, frame_rate.num };
			duration = (frame_rate.num && frame_rate.den ? av_q2d(avr) : 0);
			pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
#if 0
			if (mTargetFrame > 0 && mJumpTo > 0) {
				double fps = (double)m_is->video_st->avg_frame_rate.num / (double)m_is->video_st->avg_frame_rate.den;
				int frameidx = (int)(pts / (1. / fps));
				if (frameidx >= mTargetFrame || frame->key_frame==1) {
					mJumpTo = 0;
				}
				continue;
			}

			if (frame->key_frame == 1)
			{
				frame->key_frame = frame->key_frame;
			}
#endif
			double fps = (double)m_is->video_st->avg_frame_rate.num / (double)m_is->video_st->avg_frame_rate.den;
			int frameidx = (int)(pts / (1. / fps));
			//dmsg("queue_picture %d %d\n", frameidx, (int)(pts * 1000));
			ret = queue_picture(is, frame, pts, duration, frame->pkt_pos, is->viddec.pkt_serial);
			av_frame_unref(frame);
#if CONFIG_AVFILTER
			if (is->videoq.serial != is->viddec.pkt_serial)
				break;
		}
#endif

		if (ret < 0)
			goto the_end;
	}
the_end:
#if CONFIG_AVFILTER
	avfilter_graph_free(&graph);
#endif
	av_frame_free(&frame);
	return 0;
}

int subtitle_thread(void *arg)
{
	VideoState *is = (VideoState *)arg;
	Frame *sp;
	int got_subtitle;
	double pts;

	for (;;) {
		if (!(sp = frame_queue_peek_writable(&is->subpq)))
			return 0;

		if ((got_subtitle = decoder_decode_frame(&is->subdec, NULL, &sp->sub)) < 0)
			break;

		pts = 0;

		if (got_subtitle && sp->sub.format == 0) {
			if (sp->sub.pts != AV_NOPTS_VALUE)
				pts = sp->sub.pts / (double)AV_TIME_BASE;
			sp->pts = pts;
			sp->serial = is->subdec.pkt_serial;
			sp->width = is->subdec.avctx->width;
			sp->height = is->subdec.avctx->height;
			sp->uploaded = 0;

			/* now we can update the picture count */
			frame_queue_push(&is->subpq);
		}
		else if (got_subtitle) {
			avsubtitle_free(&sp->sub);
		}
	}
	return 0;
}

/* copy samples for viewing in editor m_sdlWnd */
void update_sample_display(VideoState *is, short *samples, int samples_size)
{
	int size, len;

	size = samples_size / sizeof(short);
	while (size > 0) {
		len = SAMPLE_ARRAY_SIZE - is->sample_array_index;
		if (len > size)
			len = size;
		memcpy(is->sample_array + is->sample_array_index, samples, len * sizeof(short));
		samples += len;
		is->sample_array_index += len;
		if (is->sample_array_index >= SAMPLE_ARRAY_SIZE)
			is->sample_array_index = 0;
		size -= len;
	}
}

/* return the wanted number of samples to get better sync if sync_type is video
 * or external master clock */
int synchronize_audio(VideoState *is, int nb_samples)
{
	int wanted_nb_samples = nb_samples;

	/* if not master, then we try to remove or add samples to correct the clock */
	if (get_master_sync_type(is) != AV_SYNC_AUDIO_MASTER) {
		double diff, avg_diff;
		int min_nb_samples, max_nb_samples;

		diff = get_clock(&is->audclk) - get_master_clock(is);

		if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
			is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
			if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
				/* not enough measures to have a correct estimate */
				is->audio_diff_avg_count++;
			}
			else {
				/* estimate the A-V difference */
				avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

				if (fabs(avg_diff) >= is->audio_diff_threshold) {
					wanted_nb_samples = nb_samples + (int)(diff * is->audio_src.freq);
					min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
					max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
					wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
				}
				av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",
					diff, avg_diff, wanted_nb_samples - nb_samples,
					is->audio_clock, is->audio_diff_threshold);
			}
		}
		else {
			/* too big difference : may be initial PTS errors, so
			   reset A-V filter */
			is->audio_diff_avg_count = 0;
			is->audio_diff_cum = 0;
		}
	}

	return wanted_nb_samples;
}

/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in is->audio_buf, with size in bytes given by the return
 * value.
 */
int audio_decode_frame(VideoState *is)
{
	int data_size, resampled_data_size;
	int64_t dec_channel_layout;
	av_unused double audio_clock0;
	int wanted_nb_samples;
	Frame *af;

	if (is->paused)
		return -1;

	do {
#if defined(_WIN32)
		while (frame_queue_nb_remaining(&is->sampq) == 0) {
			if ((av_gettime_relative() - audio_callback_time) > 1000000LL * is->audio_hw_buf_size / is->audio_tgt.bytes_per_sec / 2)
				return -1;
			av_usleep(1000);
		}
#endif
		if (!(af = frame_queue_peek_readable(&is->sampq)))
			return -1;
		frame_queue_next(&is->sampq);
	} while (af->serial != is->audioq.serial);

	data_size = av_samples_get_buffer_size(NULL, af->frame->channels,
		af->frame->nb_samples,
		(AVSampleFormat)af->frame->format, 1);

	dec_channel_layout =
		(af->frame->channel_layout && af->frame->channels == av_get_channel_layout_nb_channels(af->frame->channel_layout)) ?
		af->frame->channel_layout : av_get_default_channel_layout(af->frame->channels);
	wanted_nb_samples = synchronize_audio(is, af->frame->nb_samples);

	if (af->frame->format != is->audio_src.fmt ||
		dec_channel_layout != is->audio_src.channel_layout ||
		af->frame->sample_rate != is->audio_src.freq ||
		(wanted_nb_samples != af->frame->nb_samples && !is->swr_ctx)) {
		swr_free(&is->swr_ctx);
		is->swr_ctx = swr_alloc_set_opts(NULL,
			is->audio_tgt.channel_layout, is->audio_tgt.fmt, is->audio_tgt.freq,
			dec_channel_layout, (AVSampleFormat)af->frame->format, af->frame->sample_rate,
			0, NULL);
		if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
			av_log(NULL, AV_LOG_ERROR,
				"Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
				af->frame->sample_rate, av_get_sample_fmt_name((AVSampleFormat)af->frame->format), af->frame->channels,
				is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
			swr_free(&is->swr_ctx);
			return -1;
		}
		is->audio_src.channel_layout = dec_channel_layout;
		is->audio_src.channels = af->frame->channels;
		is->audio_src.freq = af->frame->sample_rate;
		is->audio_src.fmt = (AVSampleFormat)af->frame->format;
	}

	if (is->swr_ctx) {
		const uint8_t **in = (const uint8_t **)af->frame->extended_data;
		uint8_t **out = &is->audio_buf1;
		int out_count = (int64_t)wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256;
		int out_size = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, out_count, is->audio_tgt.fmt, 0);
		int len2;
		if (out_size < 0) {
			dmsg("av_samples_get_buffer_size() failed\n");
			return -1;
		}
		if (wanted_nb_samples != af->frame->nb_samples) {
			if (swr_set_compensation(is->swr_ctx, (wanted_nb_samples - af->frame->nb_samples) * is->audio_tgt.freq / af->frame->sample_rate,
				wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate) < 0) {
				dmsg("swr_set_compensation() failed\n");
				return -1;
			}
		}
		av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
		if (!is->audio_buf1)
			return AVERROR(ENOMEM);
		len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
		if (len2 < 0) {
			dmsg("swr_convert() failed\n");
			return -1;
		}
		if (len2 == out_count) {
			dmsg("audio buffer is probably too small\n");
			if (swr_init(is->swr_ctx) < 0)
				swr_free(&is->swr_ctx);
		}
		is->audio_buf = is->audio_buf1;
		resampled_data_size = len2 * is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
	}
	else {
		is->audio_buf = af->frame->data[0];
		resampled_data_size = data_size;
	}

	audio_clock0 = is->audio_clock;
	/* update the audio clock with the pts */
	if (!isnan(af->pts))
		is->audio_clock = af->pts + (double)af->frame->nb_samples / af->frame->sample_rate;
	else
		is->audio_clock = NAN;
	is->audio_clock_serial = af->serial;
#ifdef DEBUG
	{
		static double last_clock;
		printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n",
			is->audio_clock - last_clock,
			is->audio_clock, audio_clock0);
		last_clock = is->audio_clock;
	}
#endif
	return resampled_data_size;
}

/* prepare a new audio buffer */
void sdl_audio_callback(void *opaque, Uint8 *stream, int len)
{
	VideoState *is = (VideoState*)opaque;
	int audio_size, len1;

	audio_callback_time = av_gettime_relative();

#ifdef _DEBUGx
	static int cb_cnt = 0;	
	dmsg("> callback cnt.%d len.%d tm.%.3f\n", cb_cnt++, len, get_master_clock(is));
#endif

	while (len > 0) {
		if ((unsigned int)is->audio_buf_index >= is->audio_buf_size) {
			audio_size = audio_decode_frame(is);
			if (audio_size < 0) {
				/* if error, just output silence */
				is->audio_buf = NULL;
				is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
			}
			else {
				if (is->show_mode != SHOW_MODE_VIDEO)
					update_sample_display(is, (int16_t *)is->audio_buf, audio_size);
				is->audio_buf_size = audio_size;
			}
			is->audio_buf_index = 0;
		}
		len1 = is->audio_buf_size - is->audio_buf_index;
		if (len1 > len)
			len1 = len;
		if (!is->muted && is->audio_buf && is->audio_volume == SDL_MIX_MAXVOLUME) {
			memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
#ifdef _DEBUGx
			dmsg(">> writeaudio cnt.%d len.%d tm.%.3f\n", cb_cnt, len1, get_master_clock(is));
#endif
		}
		else {
			memset(stream, 0, len1);
			if (!is->muted && is->audio_buf) {
#ifdef _DEBUGx
				dmsg(">> mute cnt.%d len.%d tm.%.3f\n", cb_cnt, len1, get_master_clock(is));
#endif
				SDL_MixAudioFormat(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, is->audio_volume);
			}
		}
		len -= len1;
		stream += len1;
		is->audio_buf_index += len1;
	}
	is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
	/* Let's assume the audio driver that is used by SDL has two periods. */
	if (!isnan(is->audio_clock)) {
		set_clock_at(&is->audclk, is->audio_clock - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / is->audio_tgt.bytes_per_sec, is->audio_clock_serial, audio_callback_time / 1000000.0);
		sync_clock_to_slave(&is->extclk, &is->audclk);
	}
}

int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct AudioParams *audio_hw_params)
{
	SDL_AudioSpec wanted_spec, spec;
	const char *env;
	static const int next_nb_channels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };
	static const int next_sample_rates[] = { 0, 44100, 48000, 96000, 192000 };
	int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

	env = SDL_getenv("SDL_AUDIO_CHANNELS");
	if (env) {
		wanted_nb_channels = atoi(env);
		wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
	}
	if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
		wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
		wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
	}
	wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
	wanted_spec.channels = wanted_nb_channels;
	wanted_spec.freq = wanted_sample_rate;
	if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
		dmsg("Invalid sample rate or channel count!\n");
		return -1;
	}
	while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
		next_sample_rate_idx--;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.silence = 0;
	wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
	wanted_spec.callback = sdl_audio_callback;
	wanted_spec.userdata = opaque;
	while (!(audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE))) {
		dmsg("SDL_OpenAudio (%d channels, %d Hz): %s\n",
			wanted_spec.channels, wanted_spec.freq, SDL_GetError());
		wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
		if (!wanted_spec.channels) {
			wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
			wanted_spec.channels = wanted_nb_channels;
			if (!wanted_spec.freq) {
				av_log(NULL, AV_LOG_ERROR,
					"No more combinations to try, audio open failed\n");
				return -1;
			}
		}
		wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
	}
	if (spec.format != AUDIO_S16SYS) {
		av_log(NULL, AV_LOG_ERROR,
			"SDL advised audio format %d is not supported!\n", spec.format);
		return -1;
	}
	if (spec.channels != wanted_spec.channels) {
		wanted_channel_layout = av_get_default_channel_layout(spec.channels);
		if (!wanted_channel_layout) {
			av_log(NULL, AV_LOG_ERROR,
				"SDL advised channel count %d is not supported!\n", spec.channels);
			return -1;
		}
	}

	audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
	audio_hw_params->freq = spec.freq;
	audio_hw_params->channel_layout = wanted_channel_layout;
	audio_hw_params->channels = spec.channels;
	audio_hw_params->frame_size = av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
	audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
	if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
		dmsg("av_samples_get_buffer_size failed\n");
		return -1;
	}
	return spec.size;
}

/* open a given stream. Return 0 if OK */
int stream_component_open(VideoState *is, int stream_index)
{
	AVFormatContext *ic = is->ic;
	AVCodecContext *avctx;
	AVCodec *codec;
	const char *forced_codec_name = NULL;
	AVDictionary *opts = NULL;
	AVDictionaryEntry *t = NULL;
	int sample_rate, nb_channels;
	int64_t channel_layout;
	int ret = 0;
	int stream_lowres = lowres;

	if (stream_index < 0 || stream_index >= (int)ic->nb_streams)
		return -1;

	avctx = avcodec_alloc_context3(NULL);
	if (!avctx)
		return AVERROR(ENOMEM);

	ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
	if (ret < 0)
		goto fail;
	avctx->pkt_timebase = ic->streams[stream_index]->time_base;

	codec = avcodec_find_decoder(avctx->codec_id);

	#if 0
	switch (avctx->codec_type) {
	case AVMEDIA_TYPE_AUDIO: is->last_audio_stream = stream_index; forced_codec_name = audio_codec_name; break;
	case AVMEDIA_TYPE_SUBTITLE: is->last_subtitle_stream = stream_index; forced_codec_name = subtitle_codec_name; break;
	case AVMEDIA_TYPE_VIDEO: is->last_video_stream = stream_index; forced_codec_name = video_codec_name; break;
	}
	if (forced_codec_name)
		codec = avcodec_find_decoder_by_name(forced_codec_name);
	if (!codec) {
		if (forced_codec_name) av_log(NULL, AV_LOG_WARNING,
			"No codec could be found with name '%s'\n", forced_codec_name);
		else                   av_log(NULL, AV_LOG_WARNING,
			"No decoder could be found for codec %s\n", avcodec_get_name(avctx->codec_id));
		ret = AVERROR(EINVAL);
		goto fail;
	}
	#endif

	avctx->codec_id = codec->id;
	if (stream_lowres > codec->max_lowres) {
		av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
			codec->max_lowres);
		stream_lowres = codec->max_lowres;
	}
	avctx->lowres = stream_lowres;

	if (fast)
		avctx->flags2 |= AV_CODEC_FLAG2_FAST;

	#if 0
	AVDictionary *codec_opts;
	opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
	if (!av_dict_get(opts, "threads", NULL, 0))
		av_dict_set(&opts, "threads", "auto", 0);
	if (stream_lowres)
		av_dict_set_int(&opts, "lowres", stream_lowres, 0);
	if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
		av_dict_set(&opts, "refcounted_frames", "1", 0);
	if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
		goto fail;
	}
	if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
		dmsg("Option %s not found.\n", t->key);
		ret = AVERROR_OPTION_NOT_FOUND;
		goto fail;
	}
	#else
	av_dict_set(&opts, "threads", "auto", 0);
	if (stream_lowres)
		av_dict_set_int(&opts, "lowres", stream_lowres, 0);
	if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
		av_dict_set(&opts, "refcounted_frames", "1", 0);
	if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
		goto fail;
	}
	#endif

	is->eof = 0;
	ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
	switch (avctx->codec_type) {
	case AVMEDIA_TYPE_AUDIO:
#if CONFIG_AVFILTER
	{
		AVFilterContext *sink;

		is->audio_filter_src.freq = avctx->sample_rate;
		is->audio_filter_src.channels = avctx->channels;
		is->audio_filter_src.channel_layout = get_valid_channel_layout(avctx->channel_layout, avctx->channels);
		is->audio_filter_src.fmt = avctx->sample_fmt;
		if ((ret = configure_audio_filters(is, afilters, 0)) < 0)
			goto fail;
		sink = is->out_audio_filter;
		sample_rate = av_buffersink_get_sample_rate(sink);
		nb_channels = av_buffersink_get_channels(sink);
		channel_layout = av_buffersink_get_channel_layout(sink);
	}
#else
		sample_rate = avctx->sample_rate;
		nb_channels = avctx->channels;
		channel_layout = avctx->channel_layout;
#endif

		/* prepare audio output */
		if ((ret = audio_open(is, channel_layout, nb_channels, sample_rate, &is->audio_tgt)) < 0)
			goto fail;
		is->audio_hw_buf_size = ret;
		is->audio_src = is->audio_tgt;
		is->audio_buf_size = 0;
		is->audio_buf_index = 0;

		/* init averaging filter */
		is->audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
		is->audio_diff_avg_count = 0;
		/* since we do not have a precise anough audio FIFO fullness,
		   we correct audio sync only if larger than this threshold */
		is->audio_diff_threshold = (double)(is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;

		is->audio_stream = stream_index;
		is->audio_st = ic->streams[stream_index];

		decoder_init(&is->auddec, avctx, &is->audioq, is->continue_read_thread);
		if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !is->ic->iformat->read_seek) {
			is->auddec.start_pts = is->audio_st->start_time;
			is->auddec.start_pts_tb = is->audio_st->time_base;
		}
		if ((ret = decoder_start(&is->auddec, audio_thread, "audio_decoder", is)) < 0)
			goto out;
		SDL_PauseAudioDevice(audio_dev, 0);
		break;
	case AVMEDIA_TYPE_VIDEO:
		is->video_stream = stream_index;
		is->video_st = ic->streams[stream_index];

		decoder_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread);
		if ((ret = decoder_start(&is->viddec, video_thread, "video_decoder", is)) < 0)
			goto out;
		is->queue_attachments_req = 1;
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		is->subtitle_stream = stream_index;
		is->subtitle_st = ic->streams[stream_index];

		decoder_init(&is->subdec, avctx, &is->subtitleq, is->continue_read_thread);
		if ((ret = decoder_start(&is->subdec, subtitle_thread, "subtitle_decoder", is)) < 0)
			goto out;
		break;
	default:
		break;
	}
	goto out;

fail:
	avcodec_free_context(&avctx);
out:
	av_dict_free(&opts);

	return ret;
}


int decode_interrupt_cb(void *ctx)
{
	VideoState *is = (VideoState *)ctx;
	return is->abort_request;
}

int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue) {
	return stream_id < 0 ||
		queue->abort_request ||
		(st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
		queue->nb_packets > MIN_FRAMES && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}

int is_realtime(AVFormatContext *s)
{
	if (!strcmp(s->iformat->name, "rtp")
		|| !strcmp(s->iformat->name, "rtsp")
		|| !strcmp(s->iformat->name, "sdp")
		)
		return 1;

	if (s->pb && (!strncmp(s->url, "rtp:", 4)
		|| !strncmp(s->url, "udp:", 4)
		)
		)
		return 1;
	return 0;
}

void dump_metadata_ex(void *ctx, const AVDictionary *m, const char *indent)
{
	if (m && !(av_dict_count(m) == 1 && av_dict_get(m, "language", NULL, 0))) {
		const AVDictionaryEntry *tag = NULL;

		dmsg("%sMetadata:\n", indent);
		while ((tag = av_dict_get(m, "", tag, AV_DICT_IGNORE_SUFFIX)))
			if (strcmp("language", tag->key)) {
				const char *p = tag->value;
				dmsg("%s  %-16s: ", indent, tag->key);
				while (*p) {
					char tmp[256];
					size_t len = strcspn(p, "\x8\xa\xb\xc\xd");
					av_strlcpy(tmp, p, FFMIN(sizeof(tmp), len + 1));
					dmsg("%s", tmp);
					p += len;
					if (*p == 0xd) dmsg(" ");
					if (*p == 0xa) dmsg("\n%s  %-16s: ", indent, "");
					if (*p) p++;
				}
				dmsg("\n");
			}
	}
}

AVDictionary **setup_find_stream_info_opts(AVFormatContext *s, AVDictionary *codec_opts)
{	
	AVDictionary **opts;

	if (!s->nb_streams)
		return NULL;
	opts = (AVDictionary **)av_mallocz_array(s->nb_streams, sizeof(*opts));
	if (!opts) {
		av_log(NULL, AV_LOG_ERROR,
			"Could not alloc memory for stream options.\n");
		return NULL;
	}
	//int i;
	//for (i = 0; i < s->nb_streams; i++)
	//	opts[i] = NULL;// filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id, s, s->streams[i], NULL);
	return opts;
}

// Returns the estimated buffered duration in seconds (max of video / audio queues).
static double jbuf_get_duration(const VideoState *is)
{
	double d = 0.0;
	if (is->video_st && is->videoq.nb_packets > 0) {
		if (is->videoq.duration > 0)
			d = FFMAX(d, is->videoq.duration * av_q2d(is->video_st->time_base));
		else if (is->video_st->avg_frame_rate.num > 0)
			d = FFMAX(d, (double)is->videoq.nb_packets / av_q2d(is->video_st->avg_frame_rate));
	}
	if (is->audio_st && is->audioq.nb_packets > 0 && is->audioq.duration > 0)
		d = FFMAX(d, is->audioq.duration * av_q2d(is->audio_st->time_base));
	return d;
}

// Returns total queued bytes across all packet queues.
static int jbuf_get_bytes(const VideoState *is)
{
	return is->audioq.size + is->videoq.size + is->subtitleq.size;
}

// Called by read_thread after each packet is queued. Drives the state machine:
//   prebuffering=1 → wait until target duration reached → push READY event
//   prebuffering=0 → watch for underrun → push BUFFERING event
static void jbuf_update(VideoState *is)
{
	double buf_sec   = jbuf_get_duration(is);
	int    buf_bytes = jbuf_get_bytes(is);
	SDL_Event ev;

	if (is->prebuffering) {
		// Primary trigger: time target reached.
		// Fallback: byte cap exceeded AND duration is unmeasurable (no timestamps).
		int time_ready  = (buf_sec  >= g_jbuf_target_sec);
		int bytes_ready = (buf_sec  == 0.0 && is->jbuf_eff_max_bytes > 0 &&
		                   buf_bytes >= is->jbuf_eff_max_bytes);
		if (time_ready || bytes_ready) {
			is->prebuffering = 0;
			ev.type        = FF_JBUF_READY_EVENT;
			ev.user.data1  = is;
			SDL_PushEvent(&ev);
		}
	} else {
		// Playing — watch for underrun
		double thresh = g_jbuf_target_sec * g_jbuf_reopen_ratio;
		if (buf_sec < thresh && buf_sec >= 0.0 && is->video_st) {
			is->prebuffering = 1;
			ev.type        = FF_JBUF_BUFFERING_EVENT;
			ev.user.data1  = is;
			SDL_PushEvent(&ev);
		}
	}
}

int read_thread(void *arg)
{
#if  1
	VideoState *is = (VideoState *)arg;
	AVFormatContext *ic = NULL;
	int i, err, ret;
	int find_stream_info = 1;
	int st_index[AVMEDIA_TYPE_NB];
	AVPacket pkt1, *pkt = &pkt1;
	int64_t stream_start_time;
	int pkt_in_play_range = 0;
	//AVDictionaryEntry *t;
	SDL_mutex *wait_mutex = SDL_CreateMutex();
	int scan_all_pmts_set = 0;
	int64_t pkt_ts;

	if (!wait_mutex) {
		dmsg("SDL_CreateMutex(): %s\n", SDL_GetError());
		ret = AVERROR(ENOMEM);
		goto fail;
	}

	memset(st_index, -1, sizeof(st_index));
	is->eof = 0;

	ic = avformat_alloc_context();
	if (!ic) {
		dmsg("Could not allocate context.\n");
		ret = AVERROR(ENOMEM);
		goto fail;
	}
	ic->interrupt_callback.callback = decode_interrupt_cb;
	ic->interrupt_callback.opaque = is;
	if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
		av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
		scan_all_pmts_set = 1;
	}

	av_dict_set(&format_opts, "probesize", "512000", 0);
	av_dict_set(&format_opts, "analyzeduration", "1000000", 0);

	// For UDP streams with jitter buffer: override FFmpeg's internal circular buffer
	// so all buffering is visible and controlled at the application level.
	// circular_buffer_size=0 disables the hidden background receive thread.
	//if (g_jbuf_enable)
	//	av_dict_set(&format_opts, "circular_buffer_size", "0", 0);

	//err = avformat_open_input(&ic, is->filename, is->iformat, NULL);
	err = avformat_open_input(&ic, is->filename, is->iformat, &format_opts);
	if (err < 0) {
		char errbuf[256];
		av_strerror(err, errbuf, sizeof(errbuf));
		char msg[512];
		snprintf(msg, sizeof(msg), "[exMcPlayer] avformat_open_input failed: '%s' => %s\n", is->filename, errbuf);
		OutputDebugStringA(msg);
		ret = -1;
		goto fail;
	}
	/*if (scan_all_pmts_set)
		av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

	if ((t = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
		dmsg("Option %s not found.\n", t->key);
		ret = AVERROR_OPTION_NOT_FOUND;
		goto fail;
	}
	*/
	is->ic = ic;

	// Log hidden buffer sizes so the caller knows what's NOT measured by pbsdl_jbuf_getbytes().
	{
		char msg[256];
		// ② FFmpeg internal UDP circular buffer (filled by a background receive thread)
		AVIOContext *pb = ic->pb;
		int circ_buf = 0;
		if (pb && pb->opaque) {
			// av_opt_get_int works on the URLContext wrapped inside AVIOContext
			av_opt_get_int(pb, "circular_buffer_size", AV_OPT_SEARCH_CHILDREN, (int64_t*)&circ_buf);
		}
		snprintf(msg, sizeof(msg),
			"[jbuf] hidden buffers: ffmpeg_udp_circular=%d bytes, ffmpeg_io_buf=%d bytes\n",
			circ_buf, pb ? pb->buffer_size : 0);
		OutputDebugStringA(msg);
	}

	if (genpts)
		ic->flags |= AVFMT_FLAG_GENPTS;

	av_format_inject_global_side_data(ic);

	#if 1
	// Read and decode the streams to fill missing information with heuristics
	if (find_stream_info) {
		AVDictionary **opts = setup_find_stream_info_opts(ic, codec_opts);
		int orig_nb_streams = ic->nb_streams;

		err = avformat_find_stream_info(ic, opts);

		for (i = 0; i < orig_nb_streams; i++)
			av_dict_free(&opts[i]);
		av_freep(&opts);

		if (err < 0) {
			av_log(NULL, AV_LOG_WARNING,
				"%s: could not find codec parameters\n", is->filename);
			ret = -1;
			goto fail;
		}
	}
	#endif

	if (ic->pb)
		ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

	if (seek_by_bytes < 0)
		seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);

	is->max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

	//if (!window_title && (t = av_dict_get(ic->metadata, "title", NULL, 0)))
	//	window_title = av_asprintf("%s - %s", t->value, input_filename);

	/* if seeking requested, we execute it */
	if (start_time != AV_NOPTS_VALUE) {
		int64_t timestamp;

		timestamp = start_time;
		/* add the stream start time */
		if (ic->start_time != AV_NOPTS_VALUE)
			timestamp += ic->start_time;
		ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
		if (ret < 0) {
			dmsg("%s: could not seek to position %0.3f\n",
				is->filename, (double)timestamp / AV_TIME_BASE);
		}
	}

	is->realtime = is_realtime(ic);

	if (show_status)
		av_dump_format(ic, 0, is->filename, 0);

	#if 0
	for (i = 0; i < (int)ic->nb_streams; i++) {
		AVStream *st = ic->streams[i];
		enum AVMediaType type = st->codecpar->codec_type;
		st->discard = AVDISCARD_ALL;
		if (type >= 0 && /*wanted_stream_spec[type] &&*/ st_index[type] == -1)
			if (avformat_match_stream_specifier(ic, st, NULL/*wanted_stream_spec[type]*/) > 0)
				st_index[type] = i;
	}
	for (i = 0; i < AVMEDIA_TYPE_NB; i++) {
		if (/*wanted_stream_spec[i] &&*/ st_index[i] == -1) {
			//dmsg("Stream specifier %s does not match any %s stream\n", wanted_stream_spec[i], av_get_media_type_string(i));
			st_index[i] = INT_MAX;
		}
	}
	#endif

	if (!video_disable)
		st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
			st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
	if (!audio_disable)
		st_index[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
			st_index[AVMEDIA_TYPE_AUDIO], st_index[AVMEDIA_TYPE_VIDEO], NULL, 0);
	/*if (!video_disable && !subtitle_disable)
	st_index[AVMEDIA_TYPE_SUBTITLE] = av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE,
			st_index[AVMEDIA_TYPE_SUBTITLE],
			(st_index[AVMEDIA_TYPE_AUDIO] >= 0 ? st_index[AVMEDIA_TYPE_AUDIO] : st_index[AVMEDIA_TYPE_VIDEO]),
			NULL, 0);*/

	is->show_mode = (ShowMode)show_mode;
	if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
		AVStream *st = ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
		AVCodecParameters *codecpar = st->codecpar;
		AVRational sar = av_guess_sample_aspect_ratio(ic, st, NULL);
		if (codecpar->width)
			set_default_window_size(codecpar->width, codecpar->height, sar);
	}

	/* open the streams */
	if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
		stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO]);
	}

	ret = -1;
	if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
		ret = stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
	}
	if (is->show_mode == SHOW_MODE_NONE)
		is->show_mode = ret >= 0 ? SHOW_MODE_VIDEO : SHOW_MODE_RDFT;

	if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
		stream_component_open(is, st_index[AVMEDIA_TYPE_SUBTITLE]);
	}

	if (is->video_stream < 0 && is->audio_stream < 0) {
		dmsg("Failed to open file '%s' or configure filtergraph\n",
			is->filename);
		ret = -1;
		goto fail;
	}

	if (infinite_buffer < 0 && is->realtime)
		infinite_buffer = 1;

	// For realtime UDP streams with jitter buffer enabled, start in pre-buffering
	// state so the display stays frozen until enough data is queued.
	// is->paused is already 1 from stream_open; we suppress the seek-triggered
	// step_to_next_frame below until the buffer is ready.
	if (is->realtime && g_jbuf_enable) {
		is->prebuffering = 1;

		// Compute effective byte cap = target_sec × bitrate × 1.5 safety margin.
		// g_jbuf_max_bytes is the absolute ceiling (memory protection only).
		if (ic->bit_rate > 0) {
			int estimated = (int)(g_jbuf_target_sec * (double)ic->bit_rate / 8.0 * 1.5);
			is->jbuf_eff_max_bytes = FFMAX(estimated, 4 * 1024 * 1024);
		} else {
			// Bitrate unknown: use a generous 64 MB cap so time target drives
			// completion, not the byte limit
			is->jbuf_eff_max_bytes = 64 * 1024 * 1024;
		}
		// Never exceed the user-configured ceiling
		if (g_jbuf_max_bytes > 0)
			is->jbuf_eff_max_bytes = FFMIN(is->jbuf_eff_max_bytes, g_jbuf_max_bytes);
	}

	for (;;) {
		if (is->abort_request)
			break;
		if (is->paused != is->last_paused) {
			is->last_paused = is->paused;
			if (is->paused)
				is->read_pause_return = av_read_pause(ic);
			else
				av_read_play(ic);
		}
#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
		if (is->paused &&
			(!strcmp(ic->iformat->name, "rtsp") ||
			(ic->pb && !strncmp(input_filename, "mmsh:", 5)))) {
			/* wait 10 ms to avoid trying to get another packet */
			/* XXX: horrible */
			SDL_Delay(10);
			continue;
		}
#endif
		if (is->seek_req) {
			int64_t seek_target = is->seek_pos;
			int64_t seek_min = is->seek_rel > 0 ? seek_target - is->seek_rel + 2 : INT64_MIN;
			int64_t seek_max = is->seek_rel < 0 ? seek_target - is->seek_rel - 2 : INT64_MAX;
			// FIXME the +-2 is due to rounding being not done in the correct direction in generation
			//      of the seek_pos/seek_rel variables

			ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);
			//ret = av_seek_frame(is->ic, -1, seek_target, AVSEEK_FLAG_ANY);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR,
					"%s: error while seeking\n", is->ic->url);
			}
			else {
				if (is->audio_stream >= 0) {
					packet_queue_flush(&is->audioq);
					packet_queue_put(&is->audioq, &flush_pkt);
				}
				if (is->subtitle_stream >= 0) {
					packet_queue_flush(&is->subtitleq);
					packet_queue_put(&is->subtitleq, &flush_pkt);
				}
				if (is->video_stream >= 0) {
					packet_queue_flush(&is->videoq);
					packet_queue_put(&is->videoq, &flush_pkt);
				}
				if (is->seek_flags & AVSEEK_FLAG_BYTE) {
					set_clock(&is->extclk, NAN, 0);
				}
				else {
					set_clock(&is->extclk, seek_target / (double)AV_TIME_BASE, 0);
				}
			}
			is->seek_req = 0;
			is->queue_attachments_req = 1;
			is->eof = 0;
			// Don't step/unpause while jitter buffer is filling
			if (is->paused && !is->prebuffering)
				step_to_next_frame(is);
		}
		if (is->resync_req) {
			// Resuming a realtime stream after a pause: the queued packets are
			// stale (recorded while paused, now behind live). Drop them and
			// re-buffer from whatever arrives next, instead of playing through
			// the backlog — this is what keeps playback within ~target_sec of
			// live at all times. Reuses the same flush_pkt + serial-bump
			// mechanism as a seek, so already-decoded stale frames sitting in
			// pictq/sampq are recognized and skipped via serial mismatch.
			is->resync_req = 0;
			if (is->audio_stream >= 0) {
				packet_queue_flush(&is->audioq);
				packet_queue_put(&is->audioq, &flush_pkt);
			}
			if (is->subtitle_stream >= 0) {
				packet_queue_flush(&is->subtitleq);
				packet_queue_put(&is->subtitleq, &flush_pkt);
			}
			if (is->video_stream >= 0) {
				packet_queue_flush(&is->videoq);
				packet_queue_put(&is->videoq, &flush_pkt);
			}
			set_clock(&is->extclk, NAN, 0);
			is->eof = 0;
			is->queue_attachments_req = 1;
			// Stay frozen (is->paused remains 1) and re-enter pre-buffering;
			// jbuf_update() will fire FF_JBUF_READY_EVENT once ~target_sec of
			// fresh live data has accumulated, which unpauses automatically.
			is->prebuffering = 1;
		}
		if (is->queue_attachments_req) {
			if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
				AVPacket copy;
				if ((ret = av_packet_ref(&copy, &is->video_st->attached_pic)) < 0)
					goto fail;
				packet_queue_put(&is->videoq, &copy);
				packet_queue_put_nullpacket(&is->videoq, is->video_stream);
			}
			is->queue_attachments_req = 0;
		}

		/* if the queue are full, no need to read more */
		if (infinite_buffer < 1 &&
			(is->audioq.size + is->videoq.size + is->subtitleq.size > MAX_QUEUE_SIZE
				|| (stream_has_enough_packets(is->audio_st, is->audio_stream, &is->audioq) &&
					stream_has_enough_packets(is->video_st, is->video_stream, &is->videoq) &&
					stream_has_enough_packets(is->subtitle_st, is->subtitle_stream, &is->subtitleq)))) {
			/* wait 10 ms */
			SDL_LockMutex(wait_mutex);
			SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
			SDL_UnlockMutex(wait_mutex);
			continue;
		}
		/* For realtime streams (UDP/RTP), cap queue to prevent unbounded growth.
		   infinite_buffer=1 disables the MAX_QUEUE_SIZE check above, so this cap
		   must apply UNCONDITIONALLY — not just while paused/pre-buffering.
		   If it only applied during those states, the moment playback resumed
		   (paused=0, prebuffering=0) the cap would vanish and any backlog
		   would drain into videoq/audioq in one uncapped burst. Keeping it
		   active at all times paces reads to roughly the decode rate, so the
		   queue — and therefore how far behind live we can ever get — stays
		   bounded by jbuf_eff_max_bytes (≈ g_jbuf_target_sec worth of data). */
		if (is->realtime) {
			int rt_limit = (g_jbuf_enable && is->jbuf_eff_max_bytes > 0)
				? is->jbuf_eff_max_bytes
				: MAX_QUEUE_SIZE;
			if (jbuf_get_bytes(is) >= rt_limit) {
				SDL_LockMutex(wait_mutex);
				SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
				SDL_UnlockMutex(wait_mutex);
				continue;
			}
		}
		if (!is->paused &&
			(!is->audio_st || (is->auddec.finished == is->audioq.serial && frame_queue_nb_remaining(&is->sampq) == 0)) &&
			(!is->video_st || (is->viddec.finished == is->videoq.serial && frame_queue_nb_remaining(&is->pictq) == 0))) {
			if (loop != 1 && (!loop || --loop)) {
				stream_seek(is, start_time != AV_NOPTS_VALUE ? start_time : 0, 0, 0);
			}
			else if (autoexit) {
				ret = AVERROR_EOF;
				goto fail;
			}
		}
		ret = av_read_frame(ic, pkt);
		if (ret < 0) {
			if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
				if (is->video_stream >= 0)
					packet_queue_put_nullpacket(&is->videoq, is->video_stream);
				if (is->audio_stream >= 0)
					packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
				if (is->subtitle_stream >= 0)
					packet_queue_put_nullpacket(&is->subtitleq, is->subtitle_stream);
				is->eof = 1;
			}
			if (ic->pb && ic->pb->error)
				break;
			SDL_LockMutex(wait_mutex);
			SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
			SDL_UnlockMutex(wait_mutex);
			continue;
		}
		else {
			is->eof = 0;
		}
		/* check if packet is in play range specified by user, then queue, otherwise discard */
		stream_start_time = ic->streams[pkt->stream_index]->start_time;
		pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
		pkt_in_play_range = duration == AV_NOPTS_VALUE ||
			(pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
			av_q2d(ic->streams[pkt->stream_index]->time_base) -
			(double)(start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000
			<= ((double)duration / 1000000);
		if (pkt->stream_index == is->audio_stream && pkt_in_play_range) {
			packet_queue_put(&is->audioq, pkt);
		}
		else if (pkt->stream_index == is->video_stream && pkt_in_play_range
			&& !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
			packet_queue_put(&is->videoq, pkt);
		}
		else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range) {
			packet_queue_put(&is->subtitleq, pkt);
		}
		else {
			av_packet_unref(pkt);
		}

		/* Jitter buffer: update state machine after each queued packet */
		if (is->realtime && g_jbuf_enable)
			jbuf_update(is);
	}

	ret = 0;
fail:
	if (ic && !is->ic)
		avformat_close_input(&ic);

	if (ret != 0) {
		char msg[256];
		snprintf(msg, sizeof(msg), "[exMcPlayer] read_thread failed (ret=%d), pushing FF_QUIT_EVENT\n", ret);
		OutputDebugStringA(msg);
		SDL_Event event;

		event.type = FF_QUIT_EVENT;
		event.user.data1 = is;
		SDL_PushEvent(&event);
	}
	SDL_DestroyMutex(wait_mutex);
#endif
	return 0;
}

VideoState *stream_open(const char *filename, AVInputFormat *iformat, int iMute)
{
	VideoState *is;

	is = (VideoState *)av_mallocz(sizeof(VideoState));
	if (!is)
		return NULL;
	is->last_video_stream = is->video_stream = -1;
	is->last_audio_stream = is->audio_stream = -1;
	is->last_subtitle_stream = is->subtitle_stream = -1;
	is->filename = av_strdup(filename);
	if (!is->filename)
		goto fail;
	is->iformat = iformat;
	is->ytop = 0;
	is->xleft = 0;
	is->paused = 1;
	is->vp_width = 0;
	is->vp_height = 0;
	is->vp_sar.den = 1;
	is->vp_sar.num = 1;

	/* start video display */
	if (frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
		goto fail;
	if (frame_queue_init(&is->subpq, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0)
		goto fail;
	if (frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0)
		goto fail;

	if (packet_queue_init(&is->videoq) < 0 ||
		packet_queue_init(&is->audioq) < 0 ||
		packet_queue_init(&is->subtitleq) < 0)
		goto fail;

	if (!(is->continue_read_thread = SDL_CreateCond())) {
		dmsg("SDL_CreateCond(): %s\n", SDL_GetError());
		goto fail;
	}

	init_clock(&is->vidclk, &is->videoq.serial);
	init_clock(&is->audclk, &is->audioq.serial);
	init_clock(&is->extclk, &is->extclk.serial);
	is->audio_clock_serial = -1;
	//if (m_iVolume < 0)
	//	dmsg("-volume=%d < 0, setting to 0\n", m_iVolume);
	//if (m_iVolume > 100)
	//	dmsg("-volume=%d > 100, setting to 100\n", m_iVolume);
	m_iVolume = av_clip(m_iVolume, 0, 128);
	//m_iVolume = av_clip(SDL_MIX_MAXVOLUME * m_iVolume / 100, 0, SDL_MIX_MAXVOLUME);
	is->audio_volume = m_iVolume;
	is->muted = iMute;
	is->av_sync_type = AV_SYNC_AUDIO_MASTER;
	is->read_tid = SDL_CreateThread(read_thread, "read_thread", is);
	if (!is->read_tid) {
		dmsg("SDL_CreateThread(): %s\n", SDL_GetError());
	fail:
		stream_close(is);
		return NULL;
	}

	stream_seek(is, 0, 0, 0);

	return is;
}

void toggle_full_screen(VideoState *is)
{
	is_full_screen = !is_full_screen;
	SDL_SetWindowFullscreen(m_sdlWnd, is_full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void toggle_audio_display(VideoState *is)
{
	int next = is->show_mode;
	do {
		next = (next + 1) % SHOW_MODE_NB;
	} while (next != is->show_mode && (next == SHOW_MODE_VIDEO && !is->video_st || next != SHOW_MODE_VIDEO && !is->audio_st));
	if (is->show_mode != next) {
		is->force_refresh = 1;
		is->show_mode = (ShowMode)next;
	}
}

void refresh_loop_wait_event(VideoState *is, SDL_Event *event) {
	double remaining_time = 0.0;
	SDL_PumpEvents();
	while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
		if (is->abort_request)
			break;
		if (!cursor_hidden && av_gettime_relative() - cursor_last_shown > CURSOR_HIDE_DELAY) {
			//SDL_ShowCursor(0);
			cursor_hidden = 1;
		}
		if (remaining_time > 0.0)
			av_usleep((unsigned)(remaining_time * 1000000.0));
		remaining_time = REFRESH_RATE;
		if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
			video_refresh(is, &remaining_time);
		SDL_PumpEvents();
	}
}

void event_loop(VideoState *cur_stream)
{
	SDL_Event event;
#ifdef _EN_EVENT_LOOP
	double incr, pos, frac;
	int x;
	int do_loop = 1;
#endif

	for (;do_loop;) {
		if (cur_stream->abort_request)
			break;
		//double x;
		refresh_loop_wait_event(cur_stream, &event);
		switch (event.type) {
		#ifdef _EN_EVENT_LOOP
			case SDL_MOUSEBUTTONUP:
			break;
			case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q) {
				do_exit(cur_stream);
				do_loop = 0;
				break;
			}
			// If we don't yet have a m_sdlWnd, skip all key events, because read_thread might still be initializing...
			if (!cur_stream->width)
				continue;
			switch (event.key.keysym.sym) {
			case SDLK_f:
				toggle_full_screen(cur_stream);
				cur_stream->force_refresh = 1;
				break;
			case SDLK_p:
			case SDLK_SPACE:
				toggle_pause(cur_stream);
				break;
			case SDLK_m:
				toggle_mute(cur_stream);
				break;
			case SDLK_KP_MULTIPLY:
			case SDLK_0:
				update_volume(cur_stream, 1, SDL_VOLUME_STEP);
				break;
			case SDLK_KP_DIVIDE:
			case SDLK_9:
				update_volume(cur_stream, -1, SDL_VOLUME_STEP);
				break;
			case SDLK_s: // S: Step to next frame
				step_to_next_frame(cur_stream);
				break;
			case SDLK_a:
				//stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
				break;
			case SDLK_v:
				//stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
				break;
			case SDLK_c:
				//stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
				//stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
				//stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
				break;
			case SDLK_t:
				//stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
				break;
			case SDLK_w:
#if CONFIG_AVFILTER
				if (cur_stream->show_mode == SHOW_MODE_VIDEO && cur_stream->vfilter_idx < nb_vfilters - 1) {
					if (++cur_stream->vfilter_idx >= nb_vfilters)
						cur_stream->vfilter_idx = 0;
				}
				else {
					cur_stream->vfilter_idx = 0;
					toggle_audio_display(cur_stream);
				}
#else
				toggle_audio_display(cur_stream);
#endif
				break;
			case SDLK_PAGEUP:
				if (cur_stream->ic->nb_chapters <= 1) {
					incr = 600.0;
					goto do_seek;
				}
				//seek_chapter(cur_stream, 1);
				break;
			case SDLK_PAGEDOWN:
				if (cur_stream->ic->nb_chapters <= 1) {
					incr = -600.0;
					goto do_seek;
				}
				//seek_chapter(cur_stream, -1);
				break;
			case SDLK_LEFT:
				incr = seek_interval ? -seek_interval : -5.0;
				goto do_seek;
			case SDLK_RIGHT:
				//jumpTo = 100;
				incr = seek_interval ? seek_interval : 5.0;
				goto do_seek;				
			case SDLK_UP:
				incr = 60.0;
				goto do_seek;
			case SDLK_DOWN:
				incr = -60.0;
			do_seek:
				if (seek_by_bytes) {
					pos = -1;
					if (pos < 0 && cur_stream->video_stream >= 0)
						pos = (double)frame_queue_last_pos(&cur_stream->pictq);
					if (pos < 0 && cur_stream->audio_stream >= 0)
						pos = (double)frame_queue_last_pos(&cur_stream->sampq);
					if (pos < 0)
						pos = (double)avio_tell(cur_stream->ic->pb);
					if (cur_stream->ic->bit_rate)
						incr *= cur_stream->ic->bit_rate / 8.0;
					else
						incr *= 180000.0;
					pos += incr;
					stream_seek(cur_stream, (int64_t)pos, (int64_t)incr, 1);
				}
				else {
					pos = get_master_clock(cur_stream);
					if (isnan(pos))
						pos = (double)cur_stream->seek_pos / AV_TIME_BASE;
					pos += incr;
					if (cur_stream->ic->start_time != AV_NOPTS_VALUE && pos < cur_stream->ic->start_time / (double)AV_TIME_BASE)
						pos = cur_stream->ic->start_time / (double)AV_TIME_BASE;
					stream_seek(cur_stream, (int64_t)(pos * AV_TIME_BASE), (int64_t)(incr * AV_TIME_BASE), 0);
				}
				break;
			default:
				break;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			/*if (exit_on_mousedown) {
				do_exit(cur_stream);
				break;
			}*/
			if (event.button.button == SDL_BUTTON_LEFT) {
				static int64_t last_mouse_left_click = 0;
				if (av_gettime_relative() - last_mouse_left_click <= 500000) {
					toggle_full_screen(cur_stream);
					cur_stream->force_refresh = 1;
					last_mouse_left_click = 0;
				}
				else {
					last_mouse_left_click = av_gettime_relative();
				}
			}
		case SDL_MOUSEMOTION:
			if (cursor_hidden) {
				SDL_ShowCursor(1);
				cursor_hidden = 0;
			}
			cursor_last_shown = av_gettime_relative();
			if (event.type == SDL_MOUSEBUTTONDOWN) {
				if (event.button.button != SDL_BUTTON_RIGHT)
					break;
				x = event.button.x;
			}
			else {
				if (!(event.motion.state & SDL_BUTTON_RMASK))
					break;
				x = event.motion.x;
			}
			if (seek_by_bytes || cur_stream->ic->duration <= 0) {
				uint64_t size = avio_size(cur_stream->ic->pb);
				stream_seek(cur_stream, (int64_t)(size*x / cur_stream->width), 0, 1);
			}
			else {
				int64_t ts;
				int ns, hh, mm, ss;
				int tns, thh, tmm, tss;
				tns = (int)(cur_stream->ic->duration / 1000000LL);
				thh = tns / 3600;
				tmm = (tns % 3600) / 60;
				tss = (tns % 60);
				frac = x / cur_stream->width;
				ns = (int)(frac * tns);
				hh = ns / 3600;
				mm = (ns % 3600) / 60;
				ss = (ns % 60);
				av_log(NULL, AV_LOG_INFO,
					"Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac * 100,
					hh, mm, ss, thh, tmm, tss);
				ts = (int)(frac * cur_stream->ic->duration);
				if (cur_stream->ic->start_time != AV_NOPTS_VALUE)
					ts += cur_stream->ic->start_time;
				stream_seek(cur_stream, ts, 0, 0);
			}
			break;
		case SDL_QUIT:
		case FF_QUIT_EVENT:
			do_exit(cur_stream);
			do_loop = 0;
			return;
		#endif
		// Jitter buffer events — handled regardless of _EN_EVENT_LOOP
		case FF_JBUF_BUFFERING_EVENT:
			// Buffer underrun: pause output (unless user already manually paused)
			if (!cur_stream->user_paused && !cur_stream->paused)
				stream_toggle_pause(cur_stream);
			break;
		case FF_JBUF_READY_EVENT:
			// Buffer full: resume playback (only if we auto-paused, not user-paused)
			if (!cur_stream->user_paused && cur_stream->paused)
				stream_toggle_pause(cur_stream);
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				screen_width = cur_stream->width = event.window.data1;
				screen_height = cur_stream->height = event.window.data2;
				if (cur_stream->vis_texture) {
					SDL_DestroyTexture(cur_stream->vis_texture);
					cur_stream->vis_texture = NULL;
				}
				cur_stream->force_refresh = 1; //== > FLICKERING
				break;
			case SDL_WINDOWEVENT_EXPOSED:
				if (cur_stream->paused) {
					//video_display(cur_stream);
					//cur_stream->force_refresh = 1; ==> FLICKERING
				}
				break;
			}
			break;
		default:
			break;
		}
	}
}

int event_thread(void *arg)
{
	VideoState* is = (VideoState*)arg;
	event_loop(is);

	return 0;
}

int monitor_thread(void* arg)
{
	VideoState* is = (VideoState*)arg;

	while (is && !is->abort_request) {

		av_usleep((int64_t)(100 * 1000.0));	// 100ms
		double jbsec = pbsdl_jbuf_getduration();
		int    jbbyte = pbsdl_jbuf_getbytes();
		int    jbisbuffer = pbsdl_jbuf_isbuffering();
		double jbper = pbsdl_jbuf_getpercent();
		dmsg("jitter buffer = %d, %.2f sec, %d Kb, %.1f %%\n", jbisbuffer, jbsec, jbbyte/1024, jbper);
	}

	return 0;
}

#ifdef _MAIN_ENTRY
int main(int argc, char* argv[])
{
	if (argc != 2)
		return 0;

	//const char* ps = "udp://@239.10.10.10:6000";
	const char* ps = "e:\\_antigravity\\dance_1080p.mp4";
	pbsdl_load(NULL, (char*)ps, 0);

	//pbsdl_load(NULL, argv[1], 0);

	pbsdl_close();
	return 0;
}
#endif

int pbsdl_load(HWND hParWnd, char* pFile, int iMute)
{
	if (m_is) {
		pbsdl_close();
		return -1;
	}

	//flush_pkt;
	m_iVolume = 128;
	//audio_dev;
	decoder_reorder_pts = -1;
	is_full_screen = 0;
	screen_left = SDL_WINDOWPOS_CENTERED;
	screen_top = SDL_WINDOWPOS_CENTERED;

	default_width = 640;
	default_height = 480;
	screen_width = 0;
	screen_height = 0;
	display_disable = 0;
	cursor_hidden = 0;
	cursor_last_shown = 0;
	audio_callback_time = 0;
	seek_by_bytes = -1;
	rdftspeed = 0.02;
	framedrop = -1;
	show_status = -1;
	lowres = 0;
	fast = 0;
	seek_interval = 10;
	borderless = 1;
	alwaysontop = 0;
	start_time = AV_NOPTS_VALUE;
	duration = AV_NOPTS_VALUE;
	infinite_buffer = -1;
	loop = 1;
	genpts = 0;
	autoexit = 0;
	show_mode = SHOW_MODE_NONE;

#if SDL2TEST_CODE
	dmsg("SDL2 TEST\n");
	if (sdl2_init(100, 100, 800, 600) != 0) {
		sdl2_release();
		return 0;
	}	
	sdl2_begin();
	SDL_Rect rt = { 10, 10, 100, 100 };
	sdl2_rectangle(rt, 0xFF, 0xFF, 0xFF, 0xFF, true);
	sdl2_end();
	sld2_loop();
	sdl2_release();
#else
	avformat_network_init();

	//av_dict_set(&sws_dict, "flags", "bicubic", 0);

	/* Try to work around an occasional ALSA buffer underflow issue when the
		* period size is NPOT due to ALSA resampling by forcing the buffer size. */
	if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE"))
		SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE", "1", 1);
	
	Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
	if (SDL_Init(flags)) {
		dmsg("Could not initialize SDL - %s\n", SDL_GetError());
		dmsg("(Did you set the DISPLAY variable?)\n");
		return -1;
	}

	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

	av_init_packet(&flush_pkt);
	flush_pkt.data = (uint8_t *)&flush_pkt;

	if (!display_disable) {
		int flags = 0;// SDL_WINDOW_HIDDEN;
		if (alwaysontop)
#if SDL_VERSION_ATLEAST(2,0,5)
			flags |= SDL_WINDOW_ALWAYS_ON_TOP;
#endif
		if (borderless)
			flags |= SDL_WINDOW_BORDERLESS;
		else
			flags |= SDL_WINDOW_RESIZABLE;

		//flags |= SDL_WINDOW_MOUSE_FOCUS| SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_SKIP_TASKBAR | SDL_WINDOW_MOUSE_CAPTURE;
		
		if (hParWnd == NULL) {
			m_sdlWnd = SDL_CreateWindow(pFile, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, default_width, default_height, flags);
		}
		else {
			m_hParWnd = hParWnd;
			m_sdlWnd = SDL_CreateWindowFrom(hParWnd);
		}
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
		SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "direct3d11", SDL_HINT_OVERRIDE);

		if (m_sdlWnd) {
			m_sdlRend = SDL_CreateRenderer(m_sdlWnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (!m_sdlRend) {
				dmsg("Failed to initialize a hardware accelerated m_sdlRend: %s\n", SDL_GetError());
				m_sdlRend = SDL_CreateRenderer(m_sdlWnd, -1, 0);
			}
			if (m_sdlRend) {
				if (!SDL_GetRendererInfo(m_sdlRend, &m_sdlRendInfo))
					dmsg("%s\n", m_sdlRendInfo.name);
			}
		}
		if (!m_sdlWnd || !m_sdlRend || !m_sdlRendInfo.num_texture_formats) {
			dmsg("Failed to create m_sdlWnd or m_sdlRend: %s", SDL_GetError());
			do_exit(NULL);
		}
	}


	m_is = stream_open(pFile, m_avInputFmt, iMute);
	if (!m_is) {
		dmsg("Failed to initialize VideoState!\n");
		do_exit(NULL);
		return -1;
	}
#ifdef _EVENTLOOP_THREAD
	m_is->event_tid = SDL_CreateThread(event_thread, "event_thread", m_is);
	if (!m_is->event_tid) {
		dmsg("SDL_CreateThread(): %s\n", SDL_GetError());
	}
#else

	m_is->monitor_tid = SDL_CreateThread(monitor_thread, "monitor_thread", m_is);
	if (!m_is->monitor_tid) {
		dmsg("SDL_CreateThread(): %s\n", SDL_GetError());
	}

	event_loop(m_is);
#endif

#endif
	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer m_sdlWnd to add/manage files
//   2. Use the Team Explorer m_sdlWnd to connect to source control
//   3. Use the Output m_sdlWnd to see build output and other messages
//   4. Use the Error List m_sdlWnd to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

void pbsdl_close()
{
	do_exit(m_is);
	m_is = NULL;
}

// ---------------------------------------------------------------------------
// Jitter buffer API
// ---------------------------------------------------------------------------

// Configure jitter buffer before calling pbsdl_load().
//   enable        : 1 to enable, 0 to disable
//   target_sec    : pre-buffer target in seconds (e.g. 1.0)
//   max_bytes     : hard limit on packet queue memory in bytes (e.g. 4*1024*1024)
//   reopen_ratio  : fraction of target at which playback re-enters buffering (e.g. 0.3 = 30%)
void pbsdl_jbuf_config(int enable, double target_sec, int max_bytes, double reopen_ratio)
{
	g_jbuf_enable      = enable;
	g_jbuf_target_sec  = target_sec  > 0.0 ? target_sec  : 1.0;
	g_jbuf_max_bytes   = max_bytes   > 0   ? max_bytes   : 4*1024*1024;
	g_jbuf_reopen_ratio = (reopen_ratio > 0.0 && reopen_ratio < 1.0) ? reopen_ratio : 0.3;
}

// Returns currently buffered duration in seconds (video queue).
double pbsdl_jbuf_getduration()
{
	if (!m_is) return 0.0;
	return jbuf_get_duration(m_is);
}

// Returns currently buffered packet data in bytes.
int pbsdl_jbuf_getbytes()
{
	if (!m_is) return 0;
	return jbuf_get_bytes(m_is);
}

// Returns 1 if currently in pre-buffering state (output frozen), 0 otherwise.
int pbsdl_jbuf_isbuffering()
{
	if (!m_is) return 0;
	return m_is->prebuffering;
}

// Returns buffer fill as a percentage of the configured target (0–100).
// Values above 100 are clamped to 100.
double pbsdl_jbuf_getpercent()
{
	if (!m_is || g_jbuf_target_sec <= 0.0) return 0.0;
	double pct = jbuf_get_duration(m_is) / g_jbuf_target_sec * 100.0;
	return pct > 100.0 ? 100.0 : pct;
}

void pbsdl_refresh()
{
	if (m_is) {
		video_display(m_is);
		//m_is->force_refresh = 1; ==> FLICKERING
	}
}

void pbsdl_resize(int x, int y, int w, int h, int iRefresh)
{
	if (m_is) {

		if (m_hParWnd) {
			SDL_SetWindowSize(m_sdlWnd, w, h);
			SDL_SetWindowPosition(m_sdlWnd, x, y);
		}
		else {
			screen_width = m_is->width = w;
			screen_height = m_is->height = h;
			if (m_is->vis_texture) {
				SDL_DestroyTexture(m_is->vis_texture);
				m_is->vis_texture = NULL;
			}
			if (iRefresh) {
				video_display(m_is);
				//m_is->force_refresh = 1; ==> FLICKERING
			}
		}
	}
}

__int64 pbsdl_getduration()
{
	__int64 llDur = 0;

	if (m_is) {
		if (m_is->ic) {
			llDur = m_is->ic->duration;
			
			//---- ESTIMATE DURATION OF AVI FILE (2020.11.09)
			if (llDur <= 0 || llDur == AV_NOPTS_VALUE) {
				for (unsigned int i = 0; i < m_is->ic->nb_streams; i++) {
					if (m_is->ic->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
						if (m_is->ic->streams[i]->nb_frames > 0 && m_is->ic->streams[i]->time_base.den) {
							double fps = (double)m_is->ic->streams[i]->time_base.num / (double)m_is->ic->streams[i]->time_base.den;
							double dur = (double)m_is->ic->streams[i]->nb_frames * 1000000.0 * fps;
							llDur = (__int64)dur;
						}
					}
				}
			}
		}
	}

	return llDur;
}

int pbsdl_getnbframes()
{
	int nbframes = 0;

	if (m_is) {
		if (m_is->video_st) {
			nbframes = (int)m_is->video_st->nb_frames;
		}
	}

	return nbframes;
}

int pbsdl_getcurframe()
{
	int frameidx = 0;

	if (m_is) {
		if (m_is->video_st) {
			double fps = (double)m_is->video_st->avg_frame_rate.num / (double)m_is->video_st->avg_frame_rate.den;
			frameidx = (int)(pbsdl_getcurpts() / (1. / fps));
		}
	}

	return frameidx;
}

double pbsdl_getptsbyfrmidx(int frmidx)
{
	double dPts = 0;
	if (m_is) {
		if (m_is->video_st) {
			double fps = (double)m_is->video_st->avg_frame_rate.num / (double)m_is->video_st->avg_frame_rate.den;
			dPts = frmidx * (1. / fps);
		}
	}
	return dPts;
}

double pbsdl_getposition()
{
	double dPos = 0;

	if (m_is) {
		dPos = (double)frame_queue_last_pts(&m_is->pictq);
	}
	
	return dPos;
}

double pbsdl_getcurpts()
{
	if (m_is) {
		FrameQueue *f = &m_is->pictq;
		Frame *fp = &f->queue[f->rindex];
		if (f->rindex_shown && fp->serial == f->pktq->serial)
			return fp->pts;
	}
	return 0;
}

int pbsdl_getimagerect(SIZE* pSz, RECT* pRt)
{
	if (m_is) {
		pSz->cx = m_is->imgw;
		pSz->cy = m_is->imgh;

#if 1
		pRt->left = m_is->rect.x;
		pRt->top = m_is->rect.y;
		pRt->right = m_is->rect.w;
		pRt->bottom = m_is->rect.h;
#else
		if (m_is->vp_width > 0 && m_is->vp_height > 0) {
			//calculate_display_rect(&m_is->rect, pRt->left, pRt->top, pRt->right, pRt->bottom,
			//	m_is->vp_width, m_is->vp_height, m_is->vp_sar);
			AVRational aspect_ratio = m_is->vp_sar;
			int64_t width, height, x, y;
			int scr_xleft = pRt->left;
			int scr_ytop = pRt->top;
			int scr_width = pRt->right;
			int scr_height = pRt->bottom;

			if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
				aspect_ratio = av_make_q(1, 1);

			aspect_ratio = av_mul_q(aspect_ratio, av_make_q(m_is->vp_width, m_is->vp_height));

			/* XXX: we suppose the screen has a 1.0 pixel ratio */
			height = scr_height;
			width = av_rescale(height, aspect_ratio.num, aspect_ratio.den) & ~1;
			if (width > scr_width) {
				width = scr_width;
				height = av_rescale(width, aspect_ratio.den, aspect_ratio.num) & ~1;
			}
			x = (scr_width - width) / 2;
			y = (scr_height - height) / 2;
			pRt->left = (int)(scr_xleft + x);
			pRt->top = (int)(scr_ytop + y);
			pRt->right = FFMAX((int)width, 1);
			pRt->bottom = FFMAX((int)height, 1);
		}
		else {
			pRt->left = m_is->rect.x;
			pRt->top = m_is->rect.y;
			pRt->right = m_is->rect.w;
			pRt->bottom = m_is->rect.h;
		}
#endif
		return 0;
	}
	return -1;
}

void pbsdl_setrendercallback(pbsdl_rendercallback ptr)
{
	rendercallback = ptr;
}

void pbsdl_setcbreceiver(void* pcbrcv)
{
	cb_receiver = pcbrcv;
}