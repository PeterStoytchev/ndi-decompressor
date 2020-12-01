#include "FrameRecever.h"

void PFrameRecever::ReceveVideoFrame(sockpp::tcp_socket& sock, char* dataBuffer, VideoFrame* frame)
{
	processed = false;

	if (sock.read_n((void*)frame, sizeof(VideoFrame)) == -1)
	{
		printf("Failed to read video frame details!\nError: %s\n", sock.last_error_str());
	}

	if (!frame->isSingle)
	{
		auxDataBuffer = dataBuffer;
		auxFrame = *frame;

		{
			std::unique_lock<std::mutex> lk(m);
			ready = true;
		}

		cv.notify_one();
	}

	if (sock.read_n((void*)dataBuffer, frame->buf1) == -1)
	{
		printf("Failed to read video data!\nError: %s\n", sock.last_error_str());
	}

	if (!frame->isSingle)
	{
		std::unique_lock<std::mutex> lk(m);
		cv.wait(lk, [this] {return processed; });
	}
}

void PFrameRecever::ReceveVideoFrameAux()
{
	while (true)
	{
		std::unique_lock<std::mutex> lk(m);
		cv.wait(lk, [this] {return ready; });

		if (auxSocket->read_n((void*)(auxDataBuffer + auxFrame.buf1), auxFrame.buf2) != auxFrame.buf2)
		{
			printf("THREAD 2: Failed to read video data!\nError: %s\n", auxSocket->last_error_str().c_str());
		}

		processed = true;
		ready = false;

		lk.unlock();
		cv.notify_one();
	}
}

void PFrameRecever::ConfirmFrame(sockpp::tcp_socket& sock)
{
	char c = (char)7;

	if (sock.write_n(&c, sizeof(c)) != sizeof(c))
	{
		printf("Failed to send confirmation!\nError: %s\n", sock.last_error_str().c_str());
	}
}

PFrameRecever::PFrameRecever(sockpp::tcp_socket* sock)
{
	auxSocket = sock;

	//std::thread aux(&FrameRecever::ReceveVideoFrameAux, &recever, std::move(auxSocket), dataBuffer, std::ref(shouldRun), &frame);
	std::thread aux([this] 
		{
			ReceveVideoFrameAux();
		});
	aux.detach();
}

std::tuple<NDIlib_audio_frame_v2_t, float*, size_t> PFrameRecever::ReceveAudioFrame(sockpp::tcp_socket& sock)
{
	AudioFrame frame;

	if (sock.read_n((void*)&frame, sizeof(AudioFrame)) == -1)
	{
		printf("Failed to read audio frame details!\nError: %s\n", sock.last_error_str());
	}

	NDIlib_audio_frame_v2_t NDI_audio_frame;

	//TODO: pass in the buffer to avoid calling malloc every frame
	char* dataBuffer = (char*)malloc(frame.dataSize);

	if (sock.read_n((void*)dataBuffer, frame.dataSize) == -1)
	{
		printf("Failed to read audio data!\nError: %s\n", sock.last_error_str());
	}

	return std::make_tuple(frame.audioFrame, (float*)dataBuffer, frame.dataSize);
}
