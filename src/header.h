#include <iostream>
#include <thread>
#include <atomic>
#include <deque>
#include <mutex>
#include <chrono>
#include <inttypes.h>

#ifdef _DEBUG
#pragma comment(lib, "sockpp-debug")
#endif // _DEBUG

#ifndef _DEBUG
#pragma comment(lib, "sockpp-release")
#endif


#include "sockpp/tcp_acceptor.h"
#include "sockpp/version.h"

#include "Processing.NDI.Lib.h"

#include "Frame.h"

#define AUDIO_FRAME InternalAudioFrame<2 * 1024 * sizeof(float)>

#define LOG(X) std::cout << X << std::endl;


using NDI_VIDEO_FRAME = NDIlib_video_frame_v2_t;
using NDI_AUDIO_FRAME = NDIlib_audio_frame_v2_t;