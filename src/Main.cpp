#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>

#include "sockpp/tcp_acceptor.h"
#include "sockpp/version.h"

#include "Decoder.h"
#include "FrameRecever.h"

#include "Instrumentor.h"

NDIlib_send_instance_t pNDI_send;

static std::atomic<bool> exit_loop(false);
static void sigint_handler(int)
{
	exit_loop = true;
}

void SendBSFrames(VideoFrame& framePair, uint8_t* bsBuffer)
{
	PROFILE("SendBSFrame");

	NDIlib_video_frame_v2_t bsFrame = NDIlib_video_frame_v2_t(1, 1);
	bsFrame.timecode = framePair.videoFrame.timecode;
	bsFrame.timestamp = framePair.videoFrame.timestamp;
	bsFrame.p_data = bsBuffer;

	NDIlib_send_send_video_v2(pNDI_send, &bsFrame);
}

void VideoHandler(sockpp::tcp_socket sock, DecoderSettings settings)
{
	uint8_t* bsBuffer = (uint8_t*)malloc(2);
	Decoder* transcoder = new Decoder(settings);

	char* dataBuffer = (char*)malloc(settings.xres * settings.yres * 2);
	
	VideoFrame frame;

	FrameRecever::ConfirmFrame(sock);

	while (!exit_loop)
	{
		PROFILE("VideoHandler");

		FrameRecever::ReceveVideoFrame(sock, dataBuffer, &frame);

		if (frame.dataSize == 0 || frame.dataSize == 2)
		{
			SendBSFrames(frame, bsBuffer);

			printf("Buffering, sending empty!\n");
		}
		else
		{
			auto [decodedSize, decodedData] = transcoder->Decode((uint8_t*)dataBuffer, frame.dataSize);

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

		FrameRecever::ConfirmFrame(sock);
	}
}

void AudioHandler(sockpp::tcp_socket sock)
{
	while (!exit_loop)
	{
		//TODO: make the data buufer be a one time thing, that way we dont have to allocate memory every frame
		auto [NDI_audio_frame, data, dataSize] = FrameRecever::ReceveAudioFrame(sock);

		NDI_audio_frame.p_data = data;

		NDIlib_send_send_audio_v2(pNDI_send, &NDI_audio_frame);

		free(data);
	}
}

int main(int argc, char** argv)
{
	signal(SIGINT, sigint_handler);

	DecoderSettings settings;
	if (argc < 2)
	{
		settings = DecoderSettings("config.yaml");
	}
	else
	{
		settings = DecoderSettings(argv[1]);
	}

	sockpp::socket_initializer sockInit;
	sockpp::tcp_acceptor acceptor_video(settings.videoPort);
	sockpp::tcp_acceptor acceptor_audio(settings.audioPort);

	printf("Video wating on port: %d\n", settings.videoPort);
	printf("Audio wating on port: %d\n", settings.audioPort);
	
	NDIlib_send_create_t NDI_send_create_desc;
	NDI_send_create_desc.p_ndi_name = settings.srcName.c_str();
	pNDI_send = NDIlib_send_create(&NDI_send_create_desc);

	CREATE_PROFILER("ndi-server");

	while (!exit_loop) 
	{
		sockpp::inet_address peer;
		sockpp::tcp_socket video_socket = acceptor_video.accept(&peer);
		sockpp::tcp_socket audio_socket = acceptor_audio.accept(&peer);

		std::thread video_thread(VideoHandler, std::move(video_socket), settings);
		video_thread.detach();


		std::thread audio_thread(AudioHandler, std::move(audio_socket));
		audio_thread.detach();
	}

	DESTROY_PROFILER();
}