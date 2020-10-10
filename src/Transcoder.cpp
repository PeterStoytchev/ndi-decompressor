#include "Transcoder.h"

#include <assert.h>

Transcoder::Transcoder(TranscoderSettings settings)
{
	errorBuf = (char*)malloc(500);

    m_settings = settings;

	frame = av_frame_alloc();
	pkt = av_packet_alloc();

	if (m_settings.isEncoder)
	{
		codec = avcodec_find_encoder(m_settings.codecId);
		
		if (m_settings.codecId == AV_CODEC_ID_H264)
		{
			av_opt_set(codecContext->priv_data, "preset", "slow", 0);
		}
	}
	else
	{
		codec = avcodec_find_decoder(m_settings.codecId);
	}
	
	codecContext = avcodec_alloc_context3(codec);

	codecContext->bit_rate = m_settings.bitrate * 10;
	codecContext->width = m_settings.xres;
	codecContext->height = m_settings.yres;
	codecContext->time_base = { 1, m_settings.fps };
	codecContext->gop_size = m_settings.gop_size;
	codecContext->pix_fmt = m_settings.pix_fmt;
	codecContext->max_b_frames = m_settings.max_b_frames;

	swsContext = sws_getContext(m_settings.xres, m_settings.yres, AV_PIX_FMT_YUV420P, m_settings.xres, m_settings.yres, AV_PIX_FMT_UYVY422, SWS_POINT | SWS_BITEXACT, 0, 0, 0);

	frame->format = codecContext->pix_fmt;
	frame->width = codecContext->width;
	frame->height = codecContext->height;


	if (avcodec_open2(codecContext, codec, NULL) < 0)
	{
		printf("Could not open codec!\n");
		assert(0);
	}

	
	//ret = av_frame_get_buffer(frame, 32);

	//returnBufferInternal = (uint8_t*)malloc(m_settings.xres * m_settings.yres
	returnBuffer = (uint8_t*)malloc(m_settings.xres * m_settings.yres * 2);
}

std::tuple<size_t, uint8_t*> Transcoder::Encode(NDI_VIDEO_FRAME& ndi_frame)
{
	ret = av_frame_make_writable(frame);

	frame->data[0] = ndi_frame.p_data;
	frame->linesize[0] = codecContext->width * 2;

	frame->pts = i;

	if ((ret = avcodec_send_frame(codecContext, frame)) < 0)
	{
		LOG_ERR(ret);

		printf("Could not send_frame!\n");
		assert(0);
	}

	ret = avcodec_receive_packet(codecContext, pkt);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	{
		i++;
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

	return std::make_tuple(pkt->size, pkt->data);
}

std::tuple<size_t, uint8_t*> Transcoder::Decode(uint8_t* compressedData, size_t size)
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
	int linesize[3] = { m_settings.xres, m_settings.xres /2, m_settings.xres / 2 };

	uint8_t* outData[1] = { returnBuffer };
	int outLinesize[1] = { m_settings.xres * 2 };

	sws_scale(swsContext, data, linesize, 0, m_settings.yres, outData, outLinesize);

	return std::make_tuple(m_settings.xres * m_settings.yres * 2, returnBuffer);
}

void Transcoder::Repack()
{
	//memcpy(returnBufferInternal, frame->data[0], m_settings.xres * m_settings.yres);

	for (int index = 0; index < m_settings.yres * m_settings.xres; index++)
	{
		if (index % 2 == 0)
		{
			memcpy(returnBufferInternal + (m_settings.xres * m_settings.yres) + i, frame->data[1], 1);
		}
		else
		{
			memcpy(returnBufferInternal + (m_settings.xres * m_settings.yres) + i, frame->data[2], 1);
		}
	}

	

}

Transcoder::~Transcoder()
{
    
}


