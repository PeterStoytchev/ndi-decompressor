#include "FrameWrangler.h"

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

	frameSubmitter = std::thread([this] {
		HandleFrameSubmit();
	});
	frameSubmitter.detach();
}

FrameWrangler::~FrameWrangler()
{
	frameReceiver.join();
	frameDecoder.join();
	frameSubmitter.join();
}

void FrameWrangler::Stop()
{
	m_exit = true;
}

void FrameWrangler::HandleFrameReceive()
{
	while (!m_exit)
	{
		VideoFrame frame;
		FrameRecever::ReceveVideoFrame(video_socket, &frame);

		//FrameRecever::ConfirmFrame(video_socket);

		std::lock_guard<std::mutex> guard(m_receiveMutex);
		m_ReceiveQueue.push(frame);
	}
}

void FrameWrangler::HandleFrameDecode()
{
	while (!m_exit)
	{
		m_receiveMutex.lock();
		if (!m_ReceiveQueue.empty())
		{
			auto frame = m_ReceiveQueue.front();
			m_ReceiveQueue.pop();
			m_receiveMutex.unlock();

			if (frame.dataSize == 0 || frame.dataSize == 2)
			{
				free(frame.data);

				std::lock_guard<std::mutex> guard(m_submitMutex);
				SubmitBSFrame(frame);

				printf("Buffering, sending empty!\n");
			}
			else
			{
				auto [decodedSize, decodedData] = m_decoder->Decode(frame.data, frame.dataSize);
				free(frame.data);

				std::lock_guard<std::mutex> guard(m_submitMutex);
				if (decodedSize != 0)
				{
					frame.videoFrame.p_data = decodedData;
					m_SubmitQueue.push(frame.videoFrame);
				}
				else
				{
					SubmitBSFrame(frame);

					printf("Decoder is still buffering, sending empty!\n");
				}
			}
		}
		else
		{
			m_receiveMutex.unlock();
		}
	}
}

void FrameWrangler::HandleFrameSubmit()
{
	while (!m_exit)
	{
		m_submitMutex.lock();
		if (!m_SubmitQueue.empty())
		{
			auto frame = m_SubmitQueue.front();
			m_SubmitQueue.pop();
			m_submitMutex.unlock();

			NDIlib_send_send_video_v2(*pNDI_send, &frame);
		}
		else
		{
			m_submitMutex.unlock();
		}
	}

}

void FrameWrangler::SubmitBSFrame(VideoFrame& frame)
{
	NDIlib_video_frame_v2_t bsFrame = NDIlib_video_frame_v2_t(1, 1);
	bsFrame.timecode = frame.videoFrame.timecode;
	bsFrame.timestamp = frame.videoFrame.timestamp;
	bsFrame.p_data = bsBuffer;

	m_SubmitQueue.push(bsFrame);
}