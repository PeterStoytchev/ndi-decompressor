#pragma once

#include <tuple>
#include <fstream>
#include <unordered_map>
#include <assert.h>
#include <string>

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
	bool useHardwareAceel;

	int videoPort, audioPort;

	AVCodecID codecId;

	int xres, yres, fps;
	const char* srcName;
};

static std::tuple<std::string, std::string> SplitString(std::string& str, char c)
{
	int firstIndex = str.find_first_of(c);

	return std::make_tuple(str.substr(0, firstIndex), str.substr(firstIndex + 1, str.size() - firstIndex));
}

static const std::unordered_map<std::string, int> string_to_case{
   {"useHardwareAceel",1},
   {"videoPort",2},
   {"audioPort",3},
   {"codec",4},
   {"xres",5},
   {"yres",6},
   {"fps",7},
   {"srcName",8}
};

static DecoderSettings CreateSettings(const char* path)
{
	DecoderSettings localSettings;

	std::ifstream cfgFile;
	cfgFile.open(path);

	std::string line;

	//please dont look at this
	while (getline(cfgFile, line))
	{
		auto [cfgField, cfgValue] = SplitString(line, ':');

		switch (string_to_case.at(cfgField))
		{
		case 1:
		{
			if (cfgValue == std::string("true"))
			{
				localSettings.useHardwareAceel = true;
			}
			else
			{
				localSettings.useHardwareAceel = false;
			}
			break;
		}

		case 2:
		{
			localSettings.videoPort = std::atoi(cfgValue.c_str());
			break;
		}

		case 3:
		{
			localSettings.audioPort = std::atoi(cfgValue.c_str());
			break;
		}

		case 4:
		{
			if (cfgValue == std::string("H264"))
			{
				localSettings.codecId = AV_CODEC_ID_H264;
			}
			else if (cfgValue == std::string("H265"))
			{
				localSettings.codecId = AV_CODEC_ID_H265;
			}
			else
			{
				localSettings.codecId = AV_CODEC_ID_NONE;
				assert(0 && "Invalid codec provided");
			}

			break;
		}

		case 5:
		{
			localSettings.xres = std::atoi(cfgValue.c_str());
			break;
		}

		case 6:
		{
			localSettings.yres = std::atoi(cfgValue.c_str());
			break;
		}

		case 7:
		{
			localSettings.fps = std::atoi(cfgValue.c_str());
			break;
		}

		case 8:
		{
			localSettings.srcName = (char*)malloc(cfgValue.size() - 4);
			memcpy((void*)localSettings.srcName, cfgValue.c_str(), cfgValue.size());
			break;
		}

		default:
			break;
		}
	}

	return localSettings;
}



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