#include "FrameRecever.h"
#include "Profiler.h"

void FrameRecever::ReceveVideoPkt(sockpp::tcp_socket& sock, VideoPkt* frame)
{
	OPTICK_EVENT();

	if (sock.read_n((void*)frame, sizeof(VideoPkt)) == -1)
	{
		printf("Failed to read video packet size!\nError: %s\n", sock.last_error_str().c_str());
	}

	size_t dataSize = 0;
	for (int i = 0; i < 30; i++) { dataSize += frame->frameSizes[i]; }

	uint8_t* data = (uint8_t*)malloc(dataSize);

	if (sock.read_n((void*)data, dataSize) == -1)
	{
		printf("Failed to read video packet data!\nError: %s\n", sock.last_error_str().c_str());
	}

	for (int i = 0; i < 30; i++)
	{
		frame->encodedDataPackets[i] = data;
		data += frame->frameSizes[i];
	}
}

void FrameRecever::ConfirmFrame(sockpp::tcp_socket& sock)
{
	OPTICK_EVENT();

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
		printf("Failed to read audio frame details!\nError: %s\n", sock.last_error_str().c_str());
	}

	NDIlib_audio_frame_v2_t NDI_audio_frame;

	//TODO: pass in the buffer to avoid calling malloc every frame
	char* dataBuffer = (char*)malloc(frame.dataSize);

	if (sock.read_n((void*)dataBuffer, frame.dataSize) == -1)
	{
		printf("Failed to read audio data!\nError: %s\n", sock.last_error_str().c_str());
	}

	return std::make_tuple(frame.audioFrame, (float*)dataBuffer, frame.dataSize);
}
