#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <tuple>
#include <future>

#include "Decoder.h"

#include "sockpp/tcp_acceptor.h"

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
	void HandleAudio();
private:
	void ConfirmFrame();

	Decoder* m_decoder;
	NDIlib_send_instance_t* m_pNDI_send;

	std::vector<FrameBuffer*> m_frameQueue;

	std::thread m_receiverHandler;
	std::thread m_audioHandler;

	std::mutex m_swapMutex;
	std::mutex m_cvAudioMutex;
	
	std::condition_variable m_cvAudio;
	
	std::atomic<bool> m_exit = false;
	std::atomic<bool> m_audioDone = true;
	std::atomic<unsigned int> m_batchCount = 0;

	FrameBuffer* m_workingBuffer;

	sockpp::tcp_socket m_socket;
};