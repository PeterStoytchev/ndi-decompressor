#pragma once

#include <libavformat/avformat.h>

#define FRAME_BATCH_SIZE 30 //turn this into a config file variable

struct FrameBuffer
{
	size_t totalDataSize = 0;
	size_t encodedFrameSizes[FRAME_BATCH_SIZE];
	uint8_t* encodedFramePtrs[FRAME_BATCH_SIZE]; //the uint8_t* is a AVPacket* on the encoder, because the pointers are invalid when the buffer is recved, we will reuse them
	NDIlib_video_frame_v2_t videoFrames[FRAME_BATCH_SIZE];
};

struct AudioFrame
{
	size_t dataSize;
	NDIlib_audio_frame_v2_t audioFrame;
};