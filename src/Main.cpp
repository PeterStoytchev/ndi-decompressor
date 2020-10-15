#include <iostream>
#include <thread>
#include <atomic>

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

void VideoHandler(sockpp::tcp_socket sock, DecoderSettings settings)
{
	uint8_t* bsBuffer = (uint8_t*)malloc(2);

	Decoder* transcoder = new Decoder(settings);

	while (!exit_loop)
	{
		//TODO: make the data buufer be a one time thing, that way we dont have to allocate memory every frame
		auto [NDI_video_frame, dataBuffer, dataSize] = FrameRecever::ReceveVideoFrame(sock);

		if (dataSize == 0 || dataSize == 2)
		{
			NDIlib_video_frame_v2_t bsFrame = NDIlib_video_frame_v2_t(1, 1);
			bsFrame.timecode = NDI_video_frame.timecode;
			bsFrame.timestamp = NDI_video_frame.timestamp;
			bsFrame.p_data = bsBuffer;

			NDIlib_send_send_video_v2(pNDI_send, &bsFrame);

			free(dataBuffer);
			printf("Buffering, sending empty!\n");
		}
		else
		{
			auto [decodedSize, decodedData] = transcoder->Decode(dataBuffer, dataSize);

			if (decodedSize != 0)
			{
				NDI_video_frame.p_data = decodedData;
				NDIlib_send_send_video_v2(pNDI_send, &NDI_video_frame);

				free(dataBuffer);
				//free(decodedData);
			}
			else
			{
				NDIlib_video_frame_v2_t bsFrame = NDIlib_video_frame_v2_t(1, 1);
				bsFrame.timecode = NDI_video_frame.timecode;
				bsFrame.timestamp = NDI_video_frame.timestamp;
				bsFrame.p_data = bsBuffer;

				NDIlib_send_send_video_v2(pNDI_send, &bsFrame);

				free(dataBuffer);
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

	DecoderSettings settings = CreateSettings(argv[1]);
	//DecoderSettings settings = CreateSettings("C:\\Users\\Seph\\Desktop\\ndi-server\\bin\\Debug-Win32\\exampleConfig.cfg");

	sockpp::socket_initializer sockInit;

	sockpp::tcp_acceptor acceptor_video(settings.videoPort);
	sockpp::tcp_acceptor acceptor_audio(settings.audioPort);

	printf("Video wating on port: %d\n", settings.videoPort);
	printf("Audio wating on port: %d\n", settings.audioPort);

	while (true) 
	{
		sockpp::inet_address peer;
		sockpp::tcp_socket video_socket = acceptor_video.accept(&peer);
		sockpp::tcp_socket audio_socket = acceptor_audio.accept(&peer);

		std::cout << "Incoming connection from " << peer << std::endl;

		NDIlib_send_create_t NDI_send_create_desc;
		NDI_send_create_desc.p_ndi_name = settings.srcName;

		pNDI_send = NDIlib_send_create(&NDI_send_create_desc);

		std::thread video_thread(VideoHandler, std::move(video_socket), settings);
		video_thread.detach();


		std::thread audio_thread(AudioHandler, std::move(audio_socket));
		audio_thread.detach();
	}
}