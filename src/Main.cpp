#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>

#include "sockpp/tcp_acceptor.h"
#include "sockpp/version.h"

#include "Decoder.h"
#include "FrameWrangler.h"

FrameWrangler* wrangler;
NDIlib_send_instance_t pNDI_send;

static std::atomic<bool> exit_loop(false);
static void sigint_handler(int)
{
	wrangler->Stop();
	exit_loop = true;
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
	sockpp::tcp_acceptor acceptor_video(settings.dataPort);

	NDIlib_send_create_t NDI_send_create_desc;
	NDI_send_create_desc.p_ndi_name = settings.srcName.c_str();
	pNDI_send = NDIlib_send_create(&NDI_send_create_desc);

	sockpp::inet_address peer;
	printf("Wating on port: %d\n", settings.dataPort);
	
	wrangler = new FrameWrangler(settings, acceptor_video, &pNDI_send);

	delete wrangler;
}