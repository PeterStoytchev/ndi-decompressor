#include <iostream>
#include <thread>
#include <atomic>

#include "sockpp/tcp_acceptor.h"
#include "sockpp/version.h"

#include "Decoder.h"
#include "FrameRecever.h"


NDIlib_send_instance_t pNDI_send;

static std::atomic<bool> exit_loop(false);
static void sigint_handler(int)
{
	exit_loop = true;
}

void SendBSFrames(VideoFrame& framePair, uint8_t* bsBuffer)
{
	NDIlib_video_frame_v2_t bsFrame = NDIlib_video_frame_v2_t(1, 1);
	bsFrame.timecode = framePair.videoFrame.timecode;
	bsFrame.timestamp = framePair.videoFrame.timestamp;
	bsFrame.p_data = bsBuffer;

	NDIlib_send_send_video_v2(pNDI_send, &bsFrame);
}

void VideoHandler(sockpp::tcp_socket sock, sockpp::tcp_socket auxSocket, DecoderSettings settings)
{
	PFrameRecever recever(&auxSocket);
	uint8_t* bsBuffer = (uint8_t*)malloc(2);
	Decoder* transcoder = new Decoder(settings);

	char* dataBuffer = (char*)malloc(settings.xres * settings.yres * 2);
	VideoFrame frame;

	while (!exit_loop)
	{
		recever.ReceveVideoFrame(sock, dataBuffer, &frame);

		size_t totalSize = frame.buf1 + frame.buf2;
		if (totalSize == 0 || totalSize == 2 || frame.isSingle)
		{
			SendBSFrames(frame, bsBuffer);

			printf("Buffering, sending empty!\n");
		}
		else
		{
			auto [decodedSize, decodedData] = transcoder->Decode((uint8_t*)dataBuffer, totalSize);

			if (decodedSize != 0)
			{
				frame.videoFrame.p_data = decodedData;

				NDIlib_send_send_video_v2(pNDI_send, &(frame.videoFrame));
			}
			else
			{
				SendBSFrames(frame, bsBuffer);

				printf("Decoder is still buffering, sending empty!\n");
			}
		}

		recever.ConfirmFrame(sock);

		long long time_span = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - frame.frameStart).count();

		printf("Adapter time: %" PRId64 "ms\n");
	}
}

void AudioHandler(sockpp::tcp_socket sock)
{
	PFrameRecever recv;
	while (!exit_loop)
	{
		//TODO: make the data buufer be a one time thing, that way we dont have to allocate memory every frame

		auto [NDI_audio_frame, data, dataSize] = recv.ReceveAudioFrame(sock);

		NDI_audio_frame.p_data = data;

		NDIlib_send_send_audio_v2(pNDI_send, &NDI_audio_frame);

		free(data);
	}
}

int main(int argc, char** argv)
{
	if (argc == 0)
	{
		printf("No config file provided!\n");
		return -1;
	}

	DecoderSettings settings = DecoderSettings("config.yaml");

	sockpp::socket_initializer sockInit;
	sockpp::tcp_acceptor acceptor_video(settings.videoPort);
	sockpp::tcp_acceptor acceptor_audio(settings.audioPort);

	printf("Video wating on port: %d\n", settings.videoPort);
	printf("Audio wating on port: %d\n", settings.audioPort);
	
	NDIlib_send_create_t NDI_send_create_desc;
	NDI_send_create_desc.p_ndi_name = settings.srcName.c_str();
	pNDI_send = NDIlib_send_create(&NDI_send_create_desc);

	while (true) 
	{
		sockpp::inet_address peer;
		sockpp::tcp_socket video_socket = acceptor_video.accept(&peer);
		sockpp::tcp_socket video_socket2 = acceptor_video.accept(&peer);
		sockpp::tcp_socket audio_socket = acceptor_audio.accept(&peer);

		std::thread video_thread(VideoHandler, std::move(video_socket), std::move(video_socket2), settings);
		video_thread.detach();


		std::thread audio_thread(AudioHandler, std::move(audio_socket));
		audio_thread.detach();
	}
}