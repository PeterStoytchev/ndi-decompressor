#pragma once

#include <tuple>
#include <future>

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

extern "C"
{
	#include <libavutil/common.h>
}

class PFrameRecever
{
public:
	PFrameRecever() {}
	PFrameRecever(sockpp::tcp_socket* sock);
	std::tuple<NDIlib_audio_frame_v2_t, float*, size_t> ReceveAudioFrame(sockpp::tcp_socket& sock);
	void ReceveVideoFrame(sockpp::tcp_socket& sock, char* dataBuffer, VideoFrame* frame);
	void ReceveVideoFrameAux();

	void ConfirmFrame(sockpp::tcp_socket& sock);
private:
	sockpp::tcp_socket* auxSocket;
	VideoFrame auxFrame;
	char* auxDataBuffer;

	std::mutex m;
	std::condition_variable cv;
	
	bool ready = false;
	bool processed = false;
};
