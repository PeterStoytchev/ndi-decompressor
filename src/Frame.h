#pragma once

#include <iostream>
#include <Processing.NDI.Lib.h>

struct VideoFrame
{
	size_t dataSize;
	NDIlib_video_frame_v2_t videoFrame;
};

struct AudioFrame
{
	size_t dataSize;
	NDIlib_audio_frame_v2_t audioFrame;
};