#pragma once

#include "header.h"

extern "C"
{
	#include <math.h>
	#include <libavutil/opt.h>
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/channel_layout.h>
	#include <libavutil/common.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/mathematics.h>
	#include <libavutil/samplefmt.h>

	#pragma comment(lib, "avcodec")
	#pragma comment(lib, "avutil")
	#pragma comment(lib, "swscale")
}

#define LOG_ERR(ret) av_strerror(ret, errorBuf, 500); std::cout << errorBuf << std::endl; memset(errorBuf, 0, 500);

struct TranscoderSettings
{
	bool isEncoder = true;

	AVCodecID codecId;
	int64_t bitrate;

	//dont change the fileds bellow unless you have to unless you have to
	AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P; //if you use this in conjunction with OBS, using default color settings, leave this alone
	int gop_size = 10;
	int max_b_frames = 1;

	int xres = 1920;
	int yres = 1080;

	//these describe the framerate of the video, by default it is set to 60 fps
	int fps = 60;
};

class Transcoder
{
public:
	Transcoder(TranscoderSettings settings);
	~Transcoder();

	std::tuple<size_t, uint8_t*> Encode(NDI_VIDEO_FRAME& frame);
	std::tuple<size_t, uint8_t*> Decode(uint8_t* compressedData, size_t size);
private:
	void Repack();

	TranscoderSettings m_settings;
	int ret, i;

	AVCodec* codec;
	AVCodecContext * codecContext = NULL;
	
	AVFrame* frame;
	AVPacket* pkt;
	
	uint8_t* returnBufferInternal;
	uint8_t* returnBuffer;

	struct SwsContext* swsContext = NULL;

	char* errorBuf;
};