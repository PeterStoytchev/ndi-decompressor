#pragma once

#include <iostream>
#include <Processing.NDI.Lib.h>

struct VideoFramePair
{
	size_t dataSize1;
	size_t dataSize2;
	NDIlib_video_frame_v2_t videoFrame1;
	NDIlib_video_frame_v2_t videoFrame2;
};

struct AudioFrame
{
	size_t dataSize;
	NDIlib_audio_frame_v2_t audioFrame;
};