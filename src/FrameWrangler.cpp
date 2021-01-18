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

	frameDecoder = std::thread([this] {
		HandleFrameDecode();
	});
	frameDecoder.detach();
}

FrameWrangler::~FrameWrangler()
{
	frameReceiver.join();
	frameDecoder.join();
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

		std::lock_guard<std::mutex> guard(m_receiveMutex);
		m_ReceiveQueue.push_back(frame);

		/*
		if (m_ReceiveQueue.size() > 1)
		{
			printf("[HandleFrameReceive] more than one frame in m_RecieveQueue (%i)\n", m_ReceiveQueue.size());
		}
		*/
	}
}

void FrameWrangler::HandleFrameDecode()
{
	OPTICK_THREAD("HandleFrameDecode");

	std::vector<VideoFrame> localFrames;
	localFrames.reserve(60);

	while (!m_exit)
	{
		m_receiveMutex.lock();
		if (!m_ReceiveQueue.empty())
		{
			OPTICK_EVENT();

			for (VideoFrame frame : m_ReceiveQueue)
			{
				auto [decodedSize, decodedData] = m_decoder->Decode(frame.videoFrame.p_data, frame.dataSize);
				free(frame.videoFrame.p_data);

				if (decodedSize != 0)
				{
					frame.videoFrame.p_data = decodedData;
					//std::cout << "submiting frame: " << frame.videoFrame.timestamp << "\n";
					NDIlib_send_send_video_async_v2(*pNDI_send, &(frame.videoFrame));
				}
			}

			m_receiveMutex.unlock();
			FrameRecever::ConfirmFrame(video_socket);
		}
		else
		{
			m_receiveMutex.unlock();
			FrameRecever::ConfirmFrame(video_socket);
		}


	}
}