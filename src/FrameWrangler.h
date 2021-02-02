#pragma once

#include <mutex>
#include <atomic>
#include <thread>

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

	Decoder* m_decoder;
	NDIlib_send_instance_t* m_pNDI_send;

	std::vector<std::tuple<VideoPktDetails, std::vector<VideoPkt*>>> m_frameQueue; //this is kinda bad on modern CPUs, rewrite with a vector later or write custom something

	std::thread m_mainHandler;
	std::thread m_receiverHandler;

	std::mutex m_swapMutex;
	
	std::atomic<bool> m_exit = false;

	sockpp::tcp_socket m_socket;
};