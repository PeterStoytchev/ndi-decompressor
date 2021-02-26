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

	m_audioHandler = std::thread([this] {
		HandleAudio();
	});
	m_audioHandler.detach();

	Main();
}

FrameWrangler::~FrameWrangler()
{
	m_receiverHandler.join();
	m_audioHandler.join();
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
		DEBUG_LOG("[Main] Starting new iteration\n");

		m_swapMutex.lock();

		DEBUG_LOG("[Main] Got lock!\n");
		if (m_frameQueue.size() > 0)
		{
			DEBUG_LOG("[Main] There is a batch in the queue, pulling it!\n");

			m_workingBuffer  = m_frameQueue[0];

			{
				OPTICK_EVENT("VectorShift");

				//shift the vector one to the front, as if it were a queue
				std::move(m_frameQueue.begin() + 1, m_frameQueue.end(), m_frameQueue.begin());
				m_frameQueue.pop_back();
			}

			if (m_frameQueue.size() > 1)
				printf("%zu batches left in the queue\n", m_frameQueue.size());

			m_swapMutex.unlock();

			m_audioDone = false;

			DEBUG_LOG("[Main] Released lock and started audio thread!\n");

			for (int i = 0; i < FRAME_BATCH_SIZE; i++)
			{
				auto [decodedSize, decodedData] = m_decoder->Decode(m_workingBuffer->encodedVideoPtrs[i], m_workingBuffer->encodedVideoSizes[i]);

				DEBUG_LOG("[Main] Decoded a frame!\n");

				if (decodedSize != 0)
				{
					OPTICK_EVENT("SendVideoAsync");

					m_workingBuffer->ndiVideoFrames[i].p_data = decodedData;
					NDIlib_send_send_video_async_v2(*m_pNDI_send, &(m_workingBuffer->ndiVideoFrames[i]));

					DEBUG_LOG("[Main] The frame was valid and was sent to NDI!\n");
				}
				
			}

			DEBUG_LOG("[Main] Frames sent to NDI, waiting for audio!\n");

			while (m_audioDone == false);

			m_batchCount--;

			DEBUG_LOG("[Main] Audio is done, m_audioDone is set to false! Batch count --\n");

			free(m_workingBuffer);
		}
		else
		{
			OPTICK_EVENT("WaitForBatches");

			m_swapMutex.unlock();

			DEBUG_LOG("[Main] Lock released! No frames in the queue, waiting!\n");
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}
}

void FrameWrangler::HandleAudio()
{
	OPTICK_THREAD("AudioThread");
	while (!m_exit)
	{
		DEBUG_LOG("[NdiAudioThread] Starting new iteration, waiting on condition_variable!\n");
		
		/*
		std::unique_lock<std::mutex> lk(m_cvAudioMutex);
		m_cvAudio.wait(lk);
		*/
		while (m_audioDone == true);

		OPTICK_EVENT("HandleAudio");

		DEBUG_LOG("[NdiAudioThread] Condition_variable triggered!\n");

		for (int i = 0; i < FRAME_BATCH_SIZE_AUDIO; i++)
		{
			{
				OPTICK_EVENT("SendAudio");
				NDIlib_send_send_audio_v2(*m_pNDI_send, &(m_workingBuffer->ndiAudioFrames[i]));
			
				DEBUG_LOG("[NdiAudioThread] Sent frame to NDI!\n");
			}
		}

		m_audioDone = true;

		DEBUG_LOG("[NdiAudioThread] Signal that the audio thread is done!\n");
	}
}

void FrameWrangler::Receiver()
{
	OPTICK_THREAD("ReceverThread");
	while (!m_exit)
	{
		OPTICK_EVENT("Recieve");

		DEBUG_LOG("[Network] Started new iteration!\n");

		auto t1 = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
		if (m_batchCount > 2) 
		{ 
			DEBUG_LOG("[Network] More than the allowed number of batches in the buffer, waiting for clear!\n");
			while (m_batchCount != 1); //Heisenbug? 
			DEBUG_LOG("[Network] Done waiting!\n");
		}

		auto t2 = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
		auto dur = t2 - t1;

		if (dur > 1)
			std::cout << "batch loop time: " << dur << "ms \n";

		ConfirmFrame();
		DEBUG_LOG("[Network] Frame confirmation sent!\n");

		size_t dataSize = 0;
		{
			OPTICK_EVENT("RecvFrameBuffer");

			if (m_socket.read_n((void*)&dataSize, sizeof(size_t)) == -1)
			{
				printf("[DebugLog][Networking] Failed to read video packet details!\nError: %s\n", m_socket.last_error_str().c_str());
			}

			DEBUG_LOG("[Network] Recved the size of a FrameBuffer!\n");
		}
		
		uint8_t* buf = (uint8_t*)malloc(dataSize);

		{
			OPTICK_EVENT("RecvVideoData");

			//recv the the buffer
			if (m_socket.read_n((void*)buf, dataSize) != dataSize)
			{
				printf("[DebugLog][Networking] Failed to read video packet data!\nError: %s\n", m_socket.last_error_str().c_str());
			}

			DEBUG_LOG("[Network] Recved the data of a FrameBuffer!\n");
		}

		FrameBuffer* frameBuf = (FrameBuffer*)buf;
		buf += sizeof(FrameBuffer);

		DEBUG_LOG("[Network] Assigned the body of the FrameBuffer sturct!\n");

		//set the data pointers in the VideoPkt*s to their locations in buf
		for (int i = 0; i < FRAME_BATCH_SIZE; i++)
		{
			frameBuf->encodedVideoPtrs[i] = buf;
			buf += frameBuf->encodedVideoSizes[i];
		}
		DEBUG_LOG("[Network] Assigned the video pointers for the FrameBufffer struct!\n");

		for (int i = 0; i < FRAME_BATCH_SIZE_AUDIO; i++)
		{
			frameBuf->ndiAudioFrames[i].p_data = (float*)buf;
			buf += sizeof(float) * frameBuf->ndiAudioFrames[i].no_samples * frameBuf->ndiAudioFrames[i].no_channels;
		}
		DEBUG_LOG("[Network] Assigned the audio pointers for the FrameBufffer struct!\n");


		{
			OPTICK_EVENT("FrameInsertion");

			m_swapMutex.lock();
			m_frameQueue.push_back(frameBuf);
			m_swapMutex.unlock();
			m_batchCount++;
		}

		DEBUG_LOG("[Network] Inserted the FrameBufffer into the queue! Atomic batch count++\n");
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