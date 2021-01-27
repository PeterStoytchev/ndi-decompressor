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
	void Recever();

private:
	void ConfirmFrame();
	void ReceveVideoPkt();

	sockpp::tcp_socket m_socket;

	std::thread m_mainHandler;
	std::thread m_receverHandler;

	std::mutex m_swapMutex;
	std::mutex m_cvMutex;
	std::condition_variable m_cv;

	std::atomic<bool> m_isReady = false;
	std::atomic<bool> m_exit = false;

	Decoder* m_decoder;
	NDIlib_send_instance_t* m_pNDI_send;

	VideoPkt* pktFront = new VideoPkt();
	VideoPkt* pktBack = new VideoPkt();
};