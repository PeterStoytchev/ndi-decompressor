#include "Decoder.h"

#include <assert.h>

Decoder::Decoder(DecoderSettings settings)
{
	errorBuf = (char*)malloc(500);

    m_settings = settings;

	frame = av_frame_alloc();
	pkt = av_packet_alloc();

	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	
	codecContext = avcodec_alloc_context3(codec);
	
	codecContext->bit_rate = m_settings.bitrate * 10;
	codecContext->width = m_settings.xres;
	codecContext->height = m_settings.yres;
	codecContext->time_base = { 1, m_settings.fps };
	codecContext->gop_size = m_settings.gop_size;
	//codecContext->pix_fmt = m_settings.pix_fmt;
	codecContext->max_b_frames = m_settings.max_b_frames;

	//av_opt_set(codecContext->priv_data, "preset", "llhq", 0);
	//av_opt_set(codecContext->priv_data, "rc", "cbr", 0);

	swsContext = sws_getContext(m_settings.xres, m_settings.yres, AV_PIX_FMT_YUV420P, m_settings.xres, m_settings.yres, AV_PIX_FMT_UYVY422, SWS_POINT | SWS_BITEXACT, 0, 0, 0);

	frame->format = codecContext->pix_fmt;
	frame->width = codecContext->width;
	frame->height = codecContext->height;

	if (avcodec_open2(codecContext, codec, NULL) < 0)
	{
		printf("Could not open codec!\n");
		assert(0);
	}

	returnBuffer = (uint8_t*)malloc(m_settings.xres * m_settings.yres * 2);
}

std::tuple<size_t, uint8_t*> Decoder::Decode(uint8_t* compressedData, size_t size)
{
	ret = av_frame_make_writable(frame);

	pkt->data = compressedData;
	pkt->size = size;

	frame->pts = i;

	if ((ret = avcodec_send_packet(codecContext, pkt)) < 0)
	{
		LOG_ERR(ret);

		printf("Could not send_frame!\n");
		assert(0);
	}
	
	ret = avcodec_receive_frame(codecContext, frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	{
		printf("Buffering frame... send empty!");
		return std::make_tuple(0, nullptr);
	}
	else if (ret < 0)
	{
		LOG_ERR(ret);

		printf("Could not receive_packet!\n");
		assert(0);
	}


	i++;

	uint8_t* data[3] = { frame->data[0], frame->data[1], frame->data[2] };
	int linesize[3] = { m_settings.xres, m_settings.xres / 2, m_settings.xres / 2};

	uint8_t* outData[1] = { returnBuffer };
	int outLinesize[1] = { m_settings.xres * 2 };

	sws_scale(swsContext, data, linesize, 0, m_settings.yres, outData, outLinesize);

	return std::make_tuple(m_settings.xres * m_settings.yres * 2, returnBuffer);
}