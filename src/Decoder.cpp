#include "Decoder.h"

#include <assert.h>

static enum AVPixelFormat hw_pix_fmt;

static enum AVPixelFormat get_hw_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts)
{
	const enum AVPixelFormat* p;

	for (p = pix_fmts; *p != -1; p++)
	{
		if (*p == hw_pix_fmt)
		{
			return *p;
		}
	}

	return AV_PIX_FMT_NONE;
}


Decoder::Decoder(DecoderSettings settings)
{
	errorBuf = (char*)malloc(500);

    m_settings = settings;

	pkt = av_packet_alloc();
	
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	
	enum AVHWDeviceType type = av_hwdevice_find_type_by_name("d3d11va");

	for (int i = 0;; i++)
	{
		const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);

		if (!config)
		{
			printf("fuck, decoder %s doesnt support device type %s\n", codec->name, av_hwdevice_get_type_name(type));
			assert(0);
		}

		if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type)
		{
			hw_pix_fmt = config->pix_fmt;
			break;
		}
	}

	codecContext = avcodec_alloc_context3(codec);

	codecContext->bit_rate = m_settings.bitrate * 10;
	codecContext->width = m_settings.xres;
	codecContext->height = m_settings.yres;
	codecContext->time_base = { 1, m_settings.fps };
	codecContext->gop_size = m_settings.gop_size;
	codecContext->max_b_frames = m_settings.max_b_frames;
	//codecContext->pix_fmt = m_settings.pix_fmt;
	codecContext->get_format = get_hw_format;

	int err = 0;
	AVBufferRef* hw_device_ctx = NULL;

	if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0)))
	{
		printf("OOOPSS\n");
	}

	codecContext->hw_device_ctx = av_buffer_ref(hw_device_ctx);

	swsContext = sws_getContext(m_settings.xres, m_settings.yres, AV_PIX_FMT_NV12, m_settings.xres, m_settings.yres, AV_PIX_FMT_UYVY422, SWS_POINT | SWS_BITEXACT, 0, 0, 0);

	if (avcodec_open2(codecContext, codec, NULL) < 0)
	{
		printf("Could not open codec!\n");
		assert(0);
	}

	returnBuffer = (uint8_t*)_aligned_malloc(m_settings.xres * m_settings.yres * 2, 32);
}

std::tuple<size_t, uint8_t*> Decoder::Decode(uint8_t* compressedData, size_t size)
{
	if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc()))
	{
		printf("Done goofed allocating frames\n");
		assert(0);
	}

	ret = av_packet_from_data(pkt, compressedData, size);

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
		printf("Buffering frame... send empty!\n");
		av_frame_free(&frame);
		av_frame_free(&sw_frame);
		return std::make_tuple(0, nullptr);
	}
	else if (ret < 0)
	{
		LOG_ERR(ret);

		printf("Could not receive_packet!\n");
		assert(0);
	}

	if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)))
	{
		printf("done goofed GPU->CPU!\n");
		assert(0);
	}

	tmp_frame = sw_frame;

	i++;

	uint8_t* data[2] = { tmp_frame->data[0], tmp_frame->data[1] };
	int linesize[2] = { m_settings.xres, m_settings.xres};

	uint8_t* outData[1] = { returnBuffer };
	int outLinesize[1] = { m_settings.xres * 2 };

	sws_scale(swsContext, data, linesize, 0, m_settings.yres, outData, outLinesize);

	av_frame_free(&frame);
	av_frame_free(&sw_frame);

	return std::make_tuple(m_settings.xres * m_settings.yres * 2, returnBuffer);
}

