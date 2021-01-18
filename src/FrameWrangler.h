#pragma once

#include <queue>
#include <mutex>
#include <atomic>
#include <thread>

#include "Processing.NDI.Lib.h"

#include "Decoder.h"
#include "FrameRecever.h"


class FrameWrangler
{
public:
	FrameWrangler(DecoderSettings decSettings, sockpp::tcp_acceptor& acceptor_video, NDIlib_send_instance_t* pNDI_send);
	~FrameWrangler();

	void Stop();

	void HandleFrameReceive();
private:
	Decoder* m_decoder;
	NDIlib_send_instance_t* pNDI_send;

	std::thread frameReceiver;
	std::atomic<bool> m_exit = false;

	sockpp::tcp_socket video_socket;
};