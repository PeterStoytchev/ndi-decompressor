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
		OPTICK_FRAME("HandleFrameReceive");
		VideoFrame frame;
		FrameRecever::ReceveVideoFrame(video_socket, &frame);

		std::lock_guard<std::mutex> guard(m_receiveMutex);
		m_ReceiveQueue.push(frame);

		if (m_ReceiveQueue.size() > 1)
		{
			printf("[HandleFrameReceive] more than one frame in m_RecieveQueue (%i)\n", m_ReceiveQueue.size());
		}
	}
}

void FrameWrangler::HandleFrameDecode()
{
	OPTICK_THREAD("HandleFrameDecode");
	while (!m_exit)
	{
		m_receiveMutex.lock();
		if (!m_ReceiveQueue.empty())
		{
			OPTICK_EVENT();
			auto frame = m_ReceiveQueue.front();
			m_ReceiveQueue.pop();
			m_receiveMutex.unlock();

			auto [decodedSize, decodedData] = m_decoder->Decode(frame.videoFrame.p_data, frame.dataSize);
			free(frame.videoFrame.p_data);

			std::lock_guard<std::mutex> guard(m_submitMutex);
			if (decodedSize != 0)
			{
				frame.videoFrame.p_data = decodedData;
				m_SubmitQueue.push(frame.videoFrame);

				if (m_SubmitQueue.size() > 1)
				{
					printf("[HandleFrameDecode] more than one frame in m_SubmitQueue (%i)\n", m_SubmitQueue.size());
				}
			}
			else
			{
				SubmitBSFrame(frame);
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
	OPTICK_THREAD("HandleFrameSubmit");

	std::vector<NDIlib_video_frame_v2_t> localFrames;
	localFrames.reserve(60);

	while (!m_exit)
	{
		m_submitMutex.lock();
		if (!m_SubmitQueue.empty())
		{
			OPTICK_EVENT();

			for (int i = 0; i < m_SubmitQueue.size(); i++)
			{
				localFrames.push_back(m_SubmitQueue.front());
				m_SubmitQueue.pop();
			}
			m_submitMutex.unlock();

			for (NDIlib_video_frame_v2_t frame : localFrames)
			{
				NDIlib_send_send_video_async_v2(*pNDI_send, &frame);
			}

			localFrames.clear();
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