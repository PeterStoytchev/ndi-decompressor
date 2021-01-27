#include "FrameWrangler.h"
#include "Profiler.h"

FrameWrangler::FrameWrangler(DecoderSettings decSettings, sockpp::tcp_acceptor& acceptor_video, NDIlib_send_instance_t* pNDI_send)
{
	sockpp::inet_address peer;
	video_socket = acceptor_video.accept(&peer);

	m_decoder = new Decoder(decSettings);

	this->pNDI_send = pNDI_send;

	mainHandler = std::thread([this] {
		Main();
	});
	mainHandler.detach();

	receverHandler = std::thread([this] {
		Recever();
	});
	receverHandler.detach();
}

FrameWrangler::~FrameWrangler()
{
	mainHandler.join();
}

void FrameWrangler::Stop()
{
	m_exit = true;
}

void FrameWrangler::Main()
{
	while (!m_exit)
	{
		cv.notify_one();

		if (isReady)
		{
			PROFILE_FRAME("MainLoop");
			
			swapMutex.lock();

			for (int i = 0; i < 30; i++)
			{
				auto [decodedSize, decodedData] = m_decoder->Decode(pktFront->encodedDataPackets[i], pktFront->frameSizes[i]);

				if (decodedSize != 0)
				{
					PROFILE_FUNC("SendVideoAsync");

					pktFront->videoFrames[i].p_data = decodedData;
					NDIlib_send_send_video_async_v2(*pNDI_send, &(pktFront->videoFrames[i]));
				}

				if (i == 25)
				{
					isReady = false;
					cv.notify_one();
				}

			}

			auto bsFrame = NDIlib_video_frame_v2_t();
			NDIlib_send_send_video_async_v2(*pNDI_send, &bsFrame); //this is a sync event so that ndi can flush the last frame and we can free the array of recieved frames
		
			free(pktFront->encodedDataPackets[0]);
			swapMutex.unlock();
		}
	}
}

void FrameWrangler::Recever()
{
	OPTICK_THREAD("ReceverThread");
	while (!m_exit)
	{
		std::unique_lock<std::mutex> lk(cvMutex);
		cv.wait(lk);

		PROFILE_FUNC("Recieve");

		FrameRecever::ConfirmFrame(video_socket);
		FrameRecever::ReceveVideoPkt(video_socket, pktBack);

		swapMutex.lock();

		//swap the pointers
		VideoPkt* tmp = pktFront;
		pktFront = pktBack;
		pktBack = tmp;

		isReady = true;
		swapMutex.unlock();

		lk.unlock();
	}
}
