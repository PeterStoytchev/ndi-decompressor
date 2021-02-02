#include "FrameWrangler.h"
#include "Profiler.h"

FrameWrangler::FrameWrangler(DecoderSettings decSettings, sockpp::tcp_acceptor& acceptor_video, NDIlib_send_instance_t* m_pNDI_send)
{
	sockpp::inet_address peer;
	m_socket = acceptor_video.accept(&peer);

	m_decoder = new Decoder(decSettings);

	this->m_pNDI_send = m_pNDI_send;

	m_receiverHandler = std::thread([this] {
		Receiver();
	});
	m_receiverHandler.detach();

	m_mainHandler = std::thread([this] {
		Main();
	});
	m_mainHandler.detach();
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
		OPTICK_FRAME("MainLoop");

		m_swapMutex.lock();
		if (m_frameQueue.size() > 0)
		{
			auto [details, pkts] = m_frameQueue[0];

			//shift the vector one to the front, as if it were a queue
			std::move(m_frameQueue.begin() + 1, m_frameQueue.end(), m_frameQueue.begin());
			m_frameQueue.pop_back();

			printf("%zu batches left in the queue\n", m_frameQueue.size());

			m_swapMutex.unlock();

			printf("processing %zu frames\n", details.frameCount);

			for (int i = 0; i < details.frameCount; i++)
			{
				auto [decodedSize, decodedData] = m_decoder->Decode(pkts[i]->encodedDataPacket, pkts[i]->frameSize);

				if (decodedSize != 0)
				{
					OPTICK_EVENT("SendVideoAsync");

					pkts[i]->videoFrame.p_data = decodedData;
					NDIlib_send_send_video_async_v2(*m_pNDI_send, &(pkts[i]->videoFrame));
				}
			}

			auto bsFrame = NDIlib_video_frame_v2_t();
			NDIlib_send_send_video_async_v2(*m_pNDI_send, &bsFrame); //this is a sync event so that ndi can flush the last frame and we can free the array of recieved frames

			free(pkts[0]);
			pkts.clear();
		
			m_swapMutex.unlock();
		}
		else
		{
			m_swapMutex.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}
}

void FrameWrangler::Receiver()
{
	OPTICK_THREAD("ReceverThread");
	while (!m_exit)
	{
		OPTICK_EVENT("Recieve");

		//recv the video details
		VideoPktDetails details;
		if (m_socket.read_n((void*)&details, sizeof(VideoPktDetails)) == -1)
		{
			printf("[DebugLog][Networking] Failed to read video packet details!\nError: %s\n", m_socket.last_error_str().c_str());
		}

		uint8_t* buf = (uint8_t*)malloc(details.dataSize);

		//recv the the buffer
		if (m_socket.read_n((void*)buf, details.dataSize) != details.dataSize)
		{
			printf("[DebugLog][Networking] Failed to read video packet data!\nError: %s\n", m_socket.last_error_str().c_str());
		}

		std::vector<VideoPkt*> pkts;
		pkts.reserve(details.frameCount);

		//set the pointers to where the correct locations in buf
		for (int i = 0; i < details.frameCount; i++)
		{
			pkts.push_back((VideoPkt*)buf);
			buf += sizeof(VideoPkt);
		}
		
		//set the data pointers in the VideoPkt*s to their locations in buf
		for (int i = 0; i < details.frameCount; i++)
		{
			pkts[i]->encodedDataPacket = buf;
			buf += pkts[i]->frameSize;
		}
		
		m_swapMutex.lock();

		m_frameQueue.push_back(std::make_tuple(details, pkts));

		m_swapMutex.unlock();

		ConfirmFrame();
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