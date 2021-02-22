#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <tuple>
#include <future>

#include "Decoder.h"

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

	std::vector<FrameBuffer*> m_frameQueue;

	std::thread m_receiverHandler;

	std::mutex m_swapMutex;
	
	std::atomic<bool> m_exit = false;
	std::atomic<unsigned int> m_batchCount = 0;

	sockpp::tcp_socket m_socket;
};