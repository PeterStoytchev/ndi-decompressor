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

	m_receverHandler = std::thread([this] {
		Recever();
	});
	m_receverHandler.detach();
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
	while (!m_exit)
	{
		m_cv.notify_one();
		if (m_isReady)
		{
			OPTICK_FRAME("MainLoop");
			
			m_swapMutex.lock();

			for (int i = 0; i < 30; i++)
			{
				auto [decodedSize, decodedData] = m_decoder->Decode(pktFront->encodedDataPackets[i], pktFront->frameSizes[i]);

				if (decodedSize != 0)
				{
					OPTICK_EVENT("SendVideoAsync");

					pktFront->videoFrames[i].p_data = decodedData;
					NDIlib_send_send_video_async_v2(*m_pNDI_send, &(pktFront->videoFrames[i]));
				}

				if (i == 25)
				{
					m_isReady = false;
					m_cv.notify_one();
				}

			}

			auto bsFrame = NDIlib_video_frame_v2_t();
			NDIlib_send_send_video_async_v2(*m_pNDI_send, &bsFrame); //this is a sync event so that ndi can flush the last frame and we can free the array of recieved frames
		
			free(pktFront->encodedDataPackets[0]);
			m_swapMutex.unlock();
		}
	}
}

void FrameWrangler::Recever()
{
	OPTICK_THREAD("ReceverThread");
	while (!m_exit)
	{
		std::unique_lock<std::mutex> lk(m_cvMutex);
		m_cv.wait(lk);

		OPTICK_EVENT("Recieve");

		ConfirmFrame();
		ReceveVideoPkt();

		m_swapMutex.lock();

		//swap the pointers
		VideoPkt* tmp = pktFront;
		pktFront = pktBack;
		pktBack = tmp;

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

void FrameWrangler::ReceveVideoPkt()
{
	OPTICK_EVENT();

	if (m_socket.read_n((void*)pktBack, sizeof(VideoPkt)) == -1)
	{
		printf("Failed to read video packet size!\nError: %s\n", m_socket.last_error_str().c_str());
	}

	size_t dataSize = 0;
	for (int i = 0; i < 30; i++) { dataSize += pktBack->frameSizes[i]; }

	uint8_t* data = (uint8_t*)malloc(dataSize);

	if (m_socket.read_n((void*)data, dataSize) == -1)
	{
		printf("Failed to read video packet data!\nError: %s\n", m_socket.last_error_str().c_str());
	}

	for (int i = 0; i < 30; i++)
	{
		pktBack->encodedDataPackets[i] = data;
		data += pktBack->frameSizes[i];
	}
}