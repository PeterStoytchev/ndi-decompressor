#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>

#include "sockpp/tcp_acceptor.h"
#include "sockpp/version.h"

#include "Decoder.h"
#include "FrameRecever.h"
#include "FrameWrangler.h"

#include "Instrumentor.h"

FrameWrangler* wrangler;
NDIlib_send_instance_t pNDI_send;

static std::atomic<bool> exit_loop(false);
static void sigint_handler(int)
{
	wrangler->Stop();
	exit_loop = true;
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

	NDIlib_send_create_t NDI_send_create_desc;
	NDI_send_create_desc.p_ndi_name = settings.srcName.c_str();
	pNDI_send = NDIlib_send_create(&NDI_send_create_desc);

	CREATE_PROFILER("ndi-server");

	sockpp::inet_address peer;

	printf("Video wating on port: %d\n", settings.videoPort);
	printf("Audio wating on port: %d\n", settings.audioPort);

	wrangler = new FrameWrangler(settings, acceptor_video, &pNDI_send);

	sockpp::tcp_socket audio_socket = acceptor_audio.accept(&peer);
	AudioHandler(std::move(audio_socket));

	delete wrangler;

	DESTROY_PROFILER();
}