struct VideoFrame
{
	size_t dataSize = 0;
	uint8_t* data;
	NDIlib_video_frame_v2_t videoFrame;
};

struct AudioFrame
{
	size_t dataSize;
	NDIlib_audio_frame_v2_t audioFrame;
};