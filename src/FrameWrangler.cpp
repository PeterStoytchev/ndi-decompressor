#include "FrameWrangler.h"
#include "Profiler.h"

FrameWrangler::FrameWrangler(DecoderSettings decSettings, sockpp::tcp_acceptor& acceptor_video, NDIlib_send_instance_t* m_pNDI_send)
{
	sockpp::inet_address peer;
	m_socket = acceptor_video.accept(&peer);

	m_decoder = new Decoder(decSettings);

	this->m_pNDI_send = m_pNDI_send;

	m_mainHandler = std::thread([this] {
		Main();
	});
	m_mainHandler.detach();

	m_receiverHandler = std::thread([this] {
		Receiver();
	});
	m_receiverHandler.detach();
}

FrameWrangler::~FrameWrangler()
{
	m_mainHandler.join();
}

void FrameWrangler::Stop()
{
	m_exit = true;
}

void FrameWrangler::Main()
{
	RecvAndSwap();
	m_isReady = true;

	while (!m_exit)
	{
		if (m_isReady)
		{
			OPTICK_FRAME("MainLoop");
			
			m_swapMutex.lock();

			for (int i = 0; i < FRAME_BATCH_SIZE; i++)
			{
				auto [decodedSize, decodedData] = m_decoder->Decode(m_pktFront->at(i)->encodedDataPacket, m_pktFront->at(i)->frameSize);
				
				if (decodedSize != 0)
				{
					OPTICK_EVENT("SendVideoAsync");

					m_pktFront->at(i)->videoFrame.p_data = decodedData;
					NDIlib_send_send_video_async_v2(*m_pNDI_send, &(m_pktFront->at(i)->videoFrame));
				}

				if (i ==  0)
				{
					m_isReady = false;
					m_cv.notify_one();
				}

			}

			auto bsFrame = NDIlib_video_frame_v2_t();
			NDIlib_send_send_video_async_v2(*m_pNDI_send, &bsFrame); //this is a sync event so that ndi can flush the last frame and we can free the array of recieved frames
		
			m_pktFront->clear();
			m_swapMutex.unlock();
		}
	}
}

void FrameWrangler::Receiver()
{
	OPTICK_THREAD("ReceverThread");
	while (!m_exit)
	{
		std::unique_lock<std::mutex> lk(m_cvMutex);
		m_cv.wait(lk);

		OPTICK_EVENT("Recieve");

		RecvAndSwap();

		m_isReady = true;

		lk.unlock();
	}
}

void FrameWrangler::ConfirmFrame()
{
	OPTICK_EVENT();

	char c = (char)7;
	if (m_socket.write_n(&c, sizeof(c)) != sizeof(c))
	{
		printf("[DebugLog][Networking] Failed to send confirmation!\nError: %s\n", m_socket.last_error_str().c_str());
	}
}

void FrameWrangler::ReceiveVideoPkt()
{
	OPTICK_EVENT();

	size_t pktSize = 0;
	if (m_socket.read_n((void*)&pktSize, sizeof(size_t)) == -1)
	{
		printf("[DebugLog][Networking] Failed to read video packet size!\nError: %s\n", m_socket.last_error_str().c_str());
	}

	m_backBuffer->GrowIfNeeded(pktSize);

	if (m_socket.read_n((void*)m_backBuffer->m_buffer, pktSize) != pktSize)
	{
		printf("[DebugLog][Networking] Failed to read video packet data!\nError: %s\n", m_socket.last_error_str().c_str());
	}

	uint8_t* bufferPtr = m_backBuffer->m_buffer;
	for (int i = 0; i < FRAME_BATCH_SIZE; i++)
	{
		m_pktBack->push_back((VideoPkt*)bufferPtr);
		bufferPtr += sizeof(VideoPkt);
	}
	
	for (int i = 0; i < FRAME_BATCH_SIZE; i++)
	{
		m_pktBack->at(i)->encodedDataPacket = bufferPtr;
		bufferPtr += m_pktBack->at(i)->frameSize;
	}
}

void FrameWrangler::RecvAndSwap()
{
	ConfirmFrame();
	ReceiveVideoPkt();

	m_swapMutex.lock();

	//swap the video pkt pointers
	std::vector<VideoPkt*>* tmp = m_pktFront;
	m_pktFront = m_pktBack;
	m_pktBack = tmp;

	//swap the frame buffer pointers
	FrameBuffer* tmpBuffer = m_frontBuffer;
	m_frontBuffer = m_backBuffer;
	m_backBuffer = tmpBuffer;

	m_swapMutex.unlock();
}
