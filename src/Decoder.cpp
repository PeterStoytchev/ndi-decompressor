#include "Decoder.h"

#include "Profiler.h"

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

	codec = avcodec_find_decoder_by_name(m_settings.codecName.c_str());
	
	codecContext = avcodec_alloc_context3(codec);
	codecContext->width = m_settings.xres;
	codecContext->height = m_settings.yres;
	codecContext->framerate = { 1, m_settings.fps };
	
	if (m_settings.threads != 1)
	{
		codecContext->thread_count = m_settings.threads;
		codecContext->slice_count = m_settings.threads;
	}

	pkt = av_packet_alloc();
	returnBuffer = (uint8_t*)malloc(m_settings.xres * m_settings.yres * 2);

	if (m_settings.useHardwareAceel)
	{
		enum AVHWDeviceType type = av_hwdevice_find_type_by_name(m_settings.hwDeviceType.c_str());
		for (int i = 0;; i++)
		{
			const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
			if (!config)
			{
				printf("[DebugLog][Decoder] Decoder %s doesnt support device type %s\n", codec->name, av_hwdevice_get_type_name(type));
				assert(0);
			}

			if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type)
			{
				hw_pix_fmt = config->pix_fmt;
				break;
			}
		}
		codecContext->get_format = get_hw_format;

		int err = 0;
		AVBufferRef* hw_device_ctx = NULL;
		if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0)))
		{
			printf("[DebugLog][Decoder] Couldn't create hwdevice_ctx.\n");
		}
		codecContext->hw_device_ctx = av_buffer_ref(hw_device_ctx);

		swsContext = sws_getContext(m_settings.xres, m_settings.yres, AV_PIX_FMT_NV12, m_settings.xres, m_settings.yres, AV_PIX_FMT_UYVY422, SWS_POINT | SWS_BITEXACT, 0, 0, 0);
	}
	else
	{
		codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
		
		swsContext = sws_getContext(m_settings.xres, m_settings.yres, AV_PIX_FMT_YUV420P, m_settings.xres, m_settings.yres, AV_PIX_FMT_UYVY422, SWS_POINT | SWS_BITEXACT, 0, 0, 0);
	}

	for (std::pair<std::string, std::string> pair : m_settings.priv_data)
	{
		av_opt_set(codecContext->priv_data, pair.first.c_str(), pair.second.c_str(), 0);
	}

	if (avcodec_open2(codecContext, codec, NULL) < 0)
	{
		printf("[DebugLog][Decoder] Could not open codec!\n");
		assert(0);
	}
	
}

std::tuple<size_t, uint8_t*> Decoder::Decode(uint8_t* compressedData, size_t size)
{
	OPTICK_EVENT();
	if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc()))
	{
		printf("[DebugLog][Decoder] Couldn't allocate frames\n");
		assert(0);
	}

	pkt->data = compressedData;
	pkt->size = size;

	frame->pts = i;

	if ((ret = avcodec_send_packet(codecContext, pkt)) < 0)
	{
		LOG_ERR(ret);

		printf("[DebugLog][Decoder] Could not send_frame!\n");

		return std::make_tuple(0, nullptr);
	}


	ret = avcodec_receive_frame(codecContext, frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	{
		printf("[DebugLog][Decoder] Buffering frame... send empty!\n");
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

	uint8_t* outData[1] = { returnBuffer };
	int outLinesize[1] = { m_settings.xres * 2 };

	if (m_settings.useHardwareAceel)
	{
		if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)))
		{
			printf("[DebugLog][Decoder] Couldn't copy frame from GPU to CPU!\n");
			assert(0);
		}

		uint8_t* data[2] = { sw_frame->data[0], sw_frame->data[1] };
		int linesize[2] = { m_settings.xres, m_settings.xres };

		sws_scale(swsContext, data, linesize, 0, m_settings.yres, outData, outLinesize);
	}
	else
	{
		uint8_t* data[3] = { frame->data[0], frame->data[1], frame->data[2] };
		int linesize[3] = { m_settings.xres, m_settings.xres / 2, m_settings.xres / 2 };

		sws_scale(swsContext, data, linesize, 0, m_settings.yres, outData, outLinesize);
	}

	i++;

	av_frame_free(&frame);
	av_frame_free(&sw_frame);

	return std::make_tuple(m_settings.xres * m_settings.yres * 2, returnBuffer);
}