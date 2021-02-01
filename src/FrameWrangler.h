#pragma once

#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

#include "Processing.NDI.Lib.h"

#include "Decoder.h"
#include "FrameRecever.h"

struct FrameBuffer
{
	bool GrowIfNeeded(size_t potentialNewSize)
	{
		if (m_peakSize < potentialNewSize)
		{
			printf("[DebugLog][FrameBuffer] Increasing frame buffer size with id %i from %llu to %llu\n", m_id, m_peakSize, potentialNewSize);

			m_peakSize = potentialNewSize;
			m_buffer = (uint8_t*)realloc(m_buffer, m_peakSize);

			assert(m_buffer != nullptr, "[DebugLog][FrameBuffer] Failed to allocate more memory, probabbly becasue the system is out of RAM!");

			return true;
		}
	}
	
	size_t frameCount = 0;
	uint8_t* m_buffer = NULL;

private:
	size_t m_peakSize = 0;
	int m_id = rand();
};

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
	void RecvAndSwap();

	void HandleTransferAsync();

	sockpp::tcp_socket m_socket;
	sockpp::tcp_socket m_auxSocket;

	std::thread m_mainHandler;
	std::thread m_receiverHandler;
	std::thread m_tansferHandlerAsync;

	size_t m_netSize1 = 0;
	size_t m_netSize2 = 0;

	std::mutex m_cvMutex;
	std::mutex m_swapMutex;
	std::mutex m_netCvMutex;
	
	std::condition_variable m_cv;
	std::condition_variable m_netCv;
	
	std::atomic<bool> m_exit = false;
	std::atomic<bool> m_isReady = false;
	std::atomic<unsigned int> m_netCounter;

	Decoder* m_decoder;
	NDIlib_send_instance_t* m_pNDI_send;

	std::vector<VideoPkt*>* m_pktFront = new std::vector<VideoPkt*>();
	std::vector<VideoPkt*>* m_pktBack = new std::vector<VideoPkt*>();

	FrameBuffer* m_frontBuffer = new FrameBuffer();
	FrameBuffer* m_backBuffer = new FrameBuffer();

};