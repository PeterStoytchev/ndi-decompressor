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
			FrameBuffer* frames  = m_frameQueue[0];

			{
				OPTICK_EVENT("VectorShift");

				//shift the vector one to the front, as if it were a queue
				std::move(m_frameQueue.begin() + 1, m_frameQueue.end(), m_frameQueue.begin());
				m_frameQueue.pop_back();
			}
		

			if (m_frameQueue.size() > 1)
				printf("%zu batches left in the queue\n", m_frameQueue.size());

			m_swapMutex.unlock();

			for (int i = 0; i < FRAME_BATCH_SIZE; i++)
			{
				auto [decodedSize, decodedData] = m_decoder->Decode(frames->encodedFramePtrs[i], frames->encodedFrameSizes[i]);

				if (decodedSize != 0)
				{
					OPTICK_EVENT("SendVideoAsync");

					frames->videoFrames[i].p_data = decodedData;
					NDIlib_send_send_video_async_v2(*m_pNDI_send, &(frames->videoFrames[i]));
				}
				
			}

			free(frames->encodedFramePtrs[0]);
			free(frames);
			m_batchCount--;
		}
		else
		{
			OPTICK_EVENT("WaitForBatches");
			m_swapMutex.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}
}

void FrameWrangler::Receiver()
{
	OPTICK_THREAD("ReceverThread");
	while (!m_exit)
	{
		OPTICK_EVENT("Recieve");

		auto t1 = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
		if (m_batchCount > 3) { while (m_batchCount != 1); } //if there are more than x batches, wait until there are y

		auto t2 = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
		auto dur = t2 - t1;

		if (dur > 1)
			std::cout << "batch loop time: " << dur << "ms \n";

		ConfirmFrame();

		FrameBuffer* details = (FrameBuffer*)malloc(sizeof(FrameBuffer));
		{
			OPTICK_EVENT("RecvFrameBuffer");

			if (m_socket.read_n((void*)details, sizeof(FrameBuffer)) == -1)
			{
				printf("[DebugLog][Networking] Failed to read video packet details!\nError: %s\n", m_socket.last_error_str().c_str());
			}

		}
		
		uint8_t* buf = (uint8_t*)malloc(details->totalDataSize);

		{
			OPTICK_EVENT("RecvVideoData");

			//recv the the buffer
			if (m_socket.read_n((void*)buf, details->totalDataSize) != details->totalDataSize)
			{
				printf("[DebugLog][Networking] Failed to read video packet data!\nError: %s\n", m_socket.last_error_str().c_str());
			}
		}

		//set the data pointers in the VideoPkt*s to their locations in buf
		for (int i = 0; i < FRAME_BATCH_SIZE; i++)
		{
			details->encodedFramePtrs[i] = buf;
			buf += details->encodedFrameSizes[i];
		}

		{
			OPTICK_EVENT("FrameInsertion");

			m_swapMutex.lock();
			m_frameQueue.push_back(details);
			m_swapMutex.unlock();
			m_batchCount++;
		}
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