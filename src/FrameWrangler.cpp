#include "FrameWrangler.h"
#include "optik/optick.h"

FrameWrangler::FrameWrangler(DecoderSettings decSettings, sockpp::tcp_acceptor& acceptor_video, NDIlib_send_instance_t* pNDI_send)
{
	sockpp::inet_address peer;
	video_socket = acceptor_video.accept(&peer);

	m_decoder = new Decoder(decSettings);

	this->pNDI_send = pNDI_send;

	frameReceiver = std::thread([this] {
		HandleFrameReceive();
	});
	frameReceiver.detach();
}

FrameWrangler::~FrameWrangler()
{
	frameReceiver.join();
}

void FrameWrangler::Stop()
{
	m_exit = true;
}

void FrameWrangler::HandleFrameReceive()
{
	FrameRecever::ConfirmFrame(video_socket);
	while (!m_exit)
	{
		OPTICK_FRAME("HandleFrameReceive");
		
		VideoFrame frame;
		FrameRecever::ReceveVideoFrame(video_socket, &frame);

		auto [decodedSize, decodedData] = m_decoder->Decode(frame.videoFrame.p_data, frame.dataSize);
		free(frame.videoFrame.p_data);

		if (decodedSize != 0)
		{
			frame.videoFrame.p_data = decodedData;
			NDIlib_send_send_video_async_v2(*pNDI_send, &(frame.videoFrame));
		}

		FrameRecever::ConfirmFrame(video_socket);

	}
}