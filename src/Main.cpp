#include <iostream>
#include <thread>
#include <atomic>
#include <winsock.h>

#include "Processing.NDI.Lib.h"
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

void SendBSFrames(VideoFramePair& framePair, uint8_t* bsBuffer)
{
	NDIlib_video_frame_v2_t bsFrame = NDIlib_video_frame_v2_t(1, 1);
	bsFrame.timecode = framePair.videoFrame1.timecode;
	bsFrame.timestamp = framePair.videoFrame1.timestamp;
	bsFrame.p_data = bsBuffer;

	NDIlib_send_send_video_v2(pNDI_send, &bsFrame);

	bsFrame.timecode = framePair.videoFrame2.timecode;
	bsFrame.timestamp = framePair.videoFrame2.timestamp;

	NDIlib_send_send_video_v2(pNDI_send, &bsFrame);
}

void VideoHandler(sockpp::tcp_socket sock, DecoderSettings settings)
{
	uint8_t* bsBuffer = (uint8_t*)malloc(2);
	Decoder* transcoder = new Decoder(settings);

	char* dataBuffer = (char*)malloc(settings.xres * settings.yres * 4);

	while (!exit_loop)
	{
		VideoFramePair framePair = FrameRecever::ReceveVideoFrame(sock, dataBuffer);

		if (framePair.dataSize1 == 0 || framePair.dataSize1 == 2 || framePair.dataSize2 == 0 || framePair.dataSize2 == 2)
		{
			SendBSFrames(framePair, bsBuffer);

			printf("Buffering, sending empty!\n");
		}
		else
		{
			auto [decodedSize, decodedData] = transcoder->Decode((uint8_t*)dataBuffer, framePair.dataSize1);
			auto [decodedSize2, decodedData2] = transcoder->Decode((uint8_t*)dataBuffer + framePair.dataSize1, framePair.dataSize2);

			if (decodedSize != 0 || decodedSize2 != 0)
			{
				framePair.videoFrame1.p_data = decodedData;
				framePair.videoFrame2.p_data = decodedData;

				NDIlib_send_send_video_v2(pNDI_send, &(framePair.videoFrame1));
				NDIlib_send_send_video_v2(pNDI_send, &(framePair.videoFrame2));
			}
			else
			{
				SendBSFrames(framePair, bsBuffer);

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
	if (argc == 0)
	{
		printf("No config file provided!\n");
		return -1;
	}

	DecoderSettings settings = DecoderSettings("config.yaml");

	sockpp::socket_initializer sockInit;
	sockpp::tcp_acceptor acceptor_video(settings.videoPort);
	sockpp::tcp_acceptor acceptor_audio(settings.audioPort);

	//SOCKET handle = acceptor_video.handle();
	//setsockopt(handle, 2, 3, "adsd", 2);

	printf("Video wating on port: %d\n", settings.videoPort);
	printf("Audio wating on port: %d\n", settings.audioPort);

	while (true) 
	{
		sockpp::inet_address peer;
		sockpp::inet_address peer2;
		sockpp::tcp_socket video_socket = acceptor_video.accept(&peer);
		sockpp::tcp_socket audio_socket = acceptor_audio.accept(&peer2);
			
		NDIlib_send_create_t NDI_send_create_desc;
		NDI_send_create_desc.p_ndi_name = settings.srcName.c_str();

		pNDI_send = NDIlib_send_create(&NDI_send_create_desc);

		std::thread video_thread(VideoHandler, std::move(video_socket), settings);
		video_thread.detach();


		std::thread audio_thread(AudioHandler, std::move(audio_socket));
		audio_thread.detach();
	}
}