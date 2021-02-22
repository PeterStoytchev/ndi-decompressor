#pragma once

//#include <libavformat/avformat.h>

#define FRAME_BATCH_SIZE 30 //turn this into a config file variable
#define FRAME_BATCH_SIZE_AUDIO 24 //turn this into a config file variable

struct FrameBuffer
{
	size_t totalDataSize = 0;

	size_t encodedVideoSizes[FRAME_BATCH_SIZE];
	uint8_t* encodedVideoPtrs[FRAME_BATCH_SIZE]; //the uint8_t* is a AVPacket* on the encoder, because the pointers are invalid when the buffer is recved, we will reuse them
	NDIlib_video_frame_v2_t ndiVideoFrames[FRAME_BATCH_SIZE];

	size_t totalAudioSize = 0;
	NDIlib_audio_frame_v2_t ndiAudioFrames[FRAME_BATCH_SIZE_AUDIO];

	uint8_t* packedData = nullptr;
};