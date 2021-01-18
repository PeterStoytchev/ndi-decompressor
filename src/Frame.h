#pragma once

#include <libavformat/avformat.h>

struct VideoFrame
{
	size_t dataSize = 0;
	AVPacket* encodedDataPacket;
	NDIlib_video_frame_v2_t videoFrame = NDIlib_video_frame_v2_t();
};

struct AudioFrame
{
	size_t dataSize;
	NDIlib_audio_frame_v2_t audioFrame;
};