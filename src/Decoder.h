#pragma once

#include <tuple>
#include <unordered_map>
#include <assert.h>
#include <string>

#include "yaml-cpp/yaml.h"

#ifdef _DEBUG
#pragma comment(lib, "libyaml-debug")
#endif // _DEBUG

#ifndef _DEBUG
#pragma comment(lib, "libyaml-release")
#endif

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
	DecoderSettings() {}

	DecoderSettings(const DecoderSettings& settings) //this is here just to ensure that the priv data is copied right
	{
		useHardwareAceel = settings.useHardwareAceel;

		hwDeviceType = settings.hwDeviceType;
		codecName = settings.codecName;
		srcName = settings.srcName;
		dataPort = settings.dataPort;
		xres = settings.xres;
		yres = settings.yres;
		fps = settings.fps;
		threads = settings.threads;

		priv_data = settings.priv_data;
	}

	DecoderSettings(const char* path)
	{
		YAML::Node config = YAML::LoadFile(path);

		useHardwareAceel = config["useHardwareAccel"].as<bool>();

		hwDeviceType = config["hwDeviceName"].as<std::string>();
		srcName = config["srcName"].as<std::string>();
		codecName = config["codecName"].as<std::string>();


		threads = config["threads"].as<int>();
		dataPort = config["dataPort"].as<int>();


		xres = config["xres"].as<int>();
		yres = config["yres"].as<int>();
		fps = config["fps"].as<int>();

		for (const auto& kv : config["priv_data"])
		{
			priv_data.emplace(kv.first.as<std::string>(), kv.second.as<std::string>());
		}
	}

	bool useHardwareAceel;
	int dataPort, xres, yres, fps, threads;

	std::string hwDeviceType;
	std::string codecName;
	std::string srcName;

	std::unordered_map<std::string, std::string> priv_data;
};


class Decoder
{
public:
	Decoder(DecoderSettings settings);
	std::tuple<size_t, uint8_t*> Decode(uint8_t* compressedData, size_t size);
	
private:
	DecoderSettings m_settings;

	AVCodec* codec;
	AVCodecContext* codecContext = NULL;
	
	AVFrame* frame;
	AVFrame* tmp_frame = NULL;
	AVFrame* sw_frame;
	AVPacket* pkt;
	
	uint8_t* returnBuffer;

	struct SwsContext* swsContext = NULL;
	
	int ret, i;
	char* errorBuf;
};