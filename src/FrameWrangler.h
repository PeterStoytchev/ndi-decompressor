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
	void HandleFrameDecode();
	void HandleFrameSubmit();

	void SubmitBSFrame(VideoFrame& frame);

private:
	Decoder* m_decoder;

	std::thread frameReceiver;
	std::thread frameDecoder;
	
	std::vector<VideoFrame> m_ReceiveQueue;

	std::mutex m_receiveMutex;

	sockpp::tcp_socket video_socket;

	std::atomic<bool> m_exit = false;

	uint8_t* bsBuffer = (uint8_t*)malloc(2);

	NDIlib_send_instance_t* pNDI_send;

};