#include <iostream>
#include <Processing.NDI.Lib.h>

struct InternalVideoFrame
{
	size_t dataSize;
	NDIlib_video_frame_v2_t videoFrame;
};

template<size_t size>
struct InternalAudioFrame
{
	InternalAudioFrame(NDIlib_audio_frame_v2_t& frame, bool empty = false)
	{
		audioFrame = frame;
		memcpy(data, frame.p_data, sizeof(float) * frame.no_samples * frame.no_channels);
	}

	NDIlib_audio_frame_v2_t audioFrame;
	char data[size];
};