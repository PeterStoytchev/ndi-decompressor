struct VideoFrame
{
	VideoFrame() {}
	VideoFrame(size_t size, uint8_t* srcData, NDIlib_video_frame_v2_t* ndiFrame)
	{
		dataSize = size;

		data = (uint8_t*)malloc(dataSize);
		memcpy(data, srcData, dataSize);
		videoFrame = *ndiFrame;
	}

	size_t dataSize = 0;
	uint8_t* data;
	NDIlib_video_frame_v2_t videoFrame;
	int64_t id = rand();
};

struct AudioFrame
{
	size_t dataSize;
	NDIlib_audio_frame_v2_t audioFrame;
};