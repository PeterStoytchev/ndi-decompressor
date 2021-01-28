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
	m_cv.notify_one();

	while (!m_exit)
	{
		if (m_isReady)
		{
			OPTICK_FRAME("MainLoop");
			
			m_swapMutex.lock();

			for (int i = 0; i < 30; i++)
			{
				auto [decodedSize, decodedData] = m_decoder->Decode(m_pktFront->encodedDataPackets[i], m_pktFront->frameSizes[i]);

				if (decodedSize != 0)
				{
					OPTICK_EVENT("SendVideoAsync");

					m_pktFront->videoFrames[i].p_data = decodedData;
					NDIlib_send_send_video_async_v2(*m_pNDI_send, &(m_pktFront->videoFrames[i]));
				}

				if (i == 25)
				{
					m_isReady = false;
					m_cv.notify_one();
				}

			}

			auto bsFrame = NDIlib_video_frame_v2_t();
			NDIlib_send_send_video_async_v2(*m_pNDI_send, &bsFrame); //this is a sync event so that ndi can flush the last frame and we can free the array of recieved frames
		
			free(m_pktFront->encodedDataPackets[0]);
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

		ConfirmFrame();
		ReceiveVideoPkt();

		m_swapMutex.lock();

		//swap the pointers
		VideoPkt* tmp = m_pktFront;
		m_pktFront = m_pktBack;
		m_pktBack = tmp;

		m_isReady = true;
		m_swapMutex.unlock();

		lk.unlock();
	}
}

void FrameWrangler::ConfirmFrame()
{
	OPTICK_EVENT();

	char c = (char)7;
	if (m_socket.write_n(&c, sizeof(c)) != sizeof(c))
	{
		printf("Failed to send confirmation!\nError: %s\n", m_socket.last_error_str().c_str());
	}
}

void FrameWrangler::ReceiveVideoPkt()
{
	OPTICK_EVENT();

	if (m_socket.read_n((void*)m_pktBack, sizeof(VideoPkt)) == -1)
	{
		printf("Failed to read video packet size!\nError: %s\n", m_socket.last_error_str().c_str());
	}

	size_t dataSize = 0;
	for (int i = 0; i < 30; i++) { dataSize += m_pktBack->frameSizes[i]; }

	if (m_maxFrameBufferSize < dataSize)
	{
		printf("[DebugLog] Increasing buffer size from %llu to %llu\n", m_maxFrameBufferSize, dataSize);

		m_globalFrameBuffer = (uint8_t*)realloc(m_globalFrameBuffer, dataSize);
		m_maxFrameBufferSize = dataSize;

		assert(m_globalFrameBuffer != nullptr, "Failed to allocate more memory, probabbly becasue the system is out of RAM!");
	}

	if (m_socket.read_n((void*)m_globalFrameBuffer, dataSize) == -1)
	{
		printf("Failed to read video packet data!\nError: %s\n", m_socket.last_error_str().c_str());
	}

	uint8_t* bufferPtr = m_globalFrameBuffer;
	for (int i = 0; i < 30; i++)
	{
		m_pktBack->encodedDataPackets[i] = bufferPtr;
		bufferPtr += m_pktBack->frameSizes[i];
	}
}