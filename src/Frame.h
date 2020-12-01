#pragma once

#include <Processing.NDI.Lib.h>

struct VideoFrame
{
	bool isSingle = false;
	size_t buf1 = 0;
	size_t buf2 = 0;
	NDIlib_video_frame_v2_t videoFrame;
};

struct AudioFrame
{
	size_t dataSize;
	NDIlib_audio_frame_v2_t audioFrame;
};