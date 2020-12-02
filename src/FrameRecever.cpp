#include "FrameRecever.h"

void FrameRecever::ReceveVideoFrame(sockpp::tcp_socket& sock, char* dataBuffer, VideoFrame* frame)
{
	if (sock.read_n((void*)frame, sizeof(VideoFrame)) == -1)
	{
		printf("Failed to read video frame details!\nError: %s\n", sock.last_error_str());
	}

	if (sock.read_n((void*)dataBuffer, frame->dataSize) == -1)
	{
		printf("Failed to read video data!\nError: %s\n", sock.last_error_str());
	}
}

void FrameRecever::ConfirmFrame(sockpp::tcp_socket& sock)
{
	char c = (char)7;

	if (sock.write_n(&c, sizeof(c)) != sizeof(c))
	{
		printf("Failed to send confirmation!\nError: %s\n", sock.last_error_str().c_str());
	}
}

std::tuple<NDIlib_audio_frame_v2_t, float*, size_t> FrameRecever::ReceveAudioFrame(sockpp::tcp_socket& sock)
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
