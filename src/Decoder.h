#pragma once

#include <tuple>

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

#define LOG_ERR(ret) av_strerror(ret, errorBuf, 500); printf("%s\n", errorBuf); memset(errorBuf, 0, 500);

struct DecoderSettings
{
	AVCodecID codecId;

	int64_t bitrate;

	AVPixelFormat pix_fmt = AV_PIX_FMT_NV12;
	int gop_size = 30;
	int max_b_frames = 0;

	int xres = 1920;
	int yres = 1080;

	int fps = 60;


};

class Decoder
{
public:
	Decoder(DecoderSettings settings);

	std::tuple<size_t, uint8_t*> Decode(uint8_t* compressedData, size_t size);
private:
	DecoderSettings m_settings;
	int ret, i;

	AVCodec* codec;
	AVCodecContext * codecContext = NULL;
	
	AVFrame* frame;
	AVFrame* tmp_frame = NULL;
	AVFrame* sw_frame;
	AVPacket* pkt;
	
	uint8_t* returnBuffer;

	struct SwsContext* swsContext = NULL;

	char* errorBuf;
};