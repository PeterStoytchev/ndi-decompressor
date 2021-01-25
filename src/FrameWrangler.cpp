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
	FrameRecever::ConfirmFrame(video_socket);
	while (!m_exit)
	{
		PROFILE_FRAME("MainLoop");

		VideoPkt frame;
		uint8_t* data = FrameRecever::ReceveVideoPkt(video_socket, &frame);

		size_t processedData = 0;
		for (int i = 0; i < 30; i++)
		{
			auto [decodedSize, decodedData] = m_decoder->Decode(data + processedData, frame.frameSizes[i]);
			processedData += frame.frameSizes[i];

			if (decodedSize != 0)
			{
				PROFILE_FUNC("SendVideoAsync");

				frame.videoFrames[i].p_data = decodedData;
				NDIlib_send_send_video_async_v2(*pNDI_send, &(frame.videoFrames[i]));
			}

		}

		NDIlib_send_send_video_async_v2(*pNDI_send, &NDIlib_video_frame_v2_t()); //this is a sync event so that we can free the array of recieved frames
		free(data);
		
		FrameRecever::ConfirmFrame(video_socket);
	}
}