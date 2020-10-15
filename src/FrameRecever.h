#pragma once

#include <tuple>

#include "sockpp/tcp_acceptor.h"
#include "sockpp/version.h"

#ifdef _DEBUG
#pragma comment(lib, "sockpp-debug")
#endif // _DEBUG

#ifndef _DEBUG
#pragma comment(lib, "sockpp-release")
#endif


#include "Processing.NDI.Lib.h"

#include "Frame.h"

namespace FrameRecever
{
	std::tuple<NDIlib_audio_frame_v2_t, float*, size_t> ReceveAudioFrame(sockpp::tcp_socket& sock);
	std::tuple<NDIlib_video_frame_v2_t, uint8_t*, size_t> ReceveVideoFrame(sockpp::tcp_socket& sock);

	void ConfirmFrame(sockpp::tcp_socket& sock);
}