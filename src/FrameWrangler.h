#pragma once

#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

#include "Processing.NDI.Lib.h"

#include "Decoder.h"
#include "FrameRecever.h"


class FrameWrangler
{
public:
	FrameWrangler(DecoderSettings decSettings, sockpp::tcp_acceptor& acceptor_video, NDIlib_send_instance_t* m_pNDI_send);
	~FrameWrangler();

	void Stop();

	void Main();
	void Receiver();

private:
	void ConfirmFrame();
	void ReceiveVideoPkt();

	sockpp::tcp_socket m_socket;

	std::thread m_mainHandler;
	std::thread m_receiverHandler;

	std::mutex m_swapMutex;
	std::mutex m_cvMutex;
	std::condition_variable m_cv;

	std::atomic<bool> m_isReady = false;
	std::atomic<bool> m_exit = false;

	Decoder* m_decoder;
	NDIlib_send_instance_t* m_pNDI_send;

	VideoPkt* m_pktFront = new VideoPkt();
	VideoPkt* m_pktBack = new VideoPkt();

	size_t m_maxFrameBufferSize = 0;
	uint8_t* m_globalFrameBuffer = NULL;
};