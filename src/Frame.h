#pragma once

#include <Processing.NDI.Lib.h>

struct VideoFrame
{
	size_t buf1 = 0;
	size_t buf2 = 0;
	bool isSingle = false;
	NDIlib_video_frame_v2_t videoFrame;
	std::chrono::time_point<std::chrono::steady_clock> frameStart;
};

struct AudioFrame
{
	size_t dataSize;
	NDIlib_audio_frame_v2_t audioFrame;
};