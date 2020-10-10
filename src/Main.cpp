#include "header.h"

#include "Decoder.h"
#include "FrameRecever.h"

std::atomic<NDIlib_send_instance_t> pNDI_send;


static std::atomic<bool> exit_loop(false);
static void sigint_handler(int)
{
	exit_loop = true;
}


void VideoHandler(sockpp::tcp_socket sock)
{
	DecoderSettings settings;
	settings.codecId = AV_CODEC_ID_H264;

	Decoder* transcoder = new Decoder(settings);

	while (!exit_loop)
	{
		auto [NDI_video_frame, dataBuffer, dataSize] = FrameRecever::ReceveVideoFrame(sock);

		auto [decodedSize, decodedData] = transcoder->Decode(dataBuffer, dataSize);

		if (decodedSize != 0)
		{
			NDI_video_frame.p_data = decodedData;

			NDIlib_send_send_video_v2(pNDI_send, &NDI_video_frame);
		}

		free(dataBuffer);
	}
}

void AudioHandler(sockpp::tcp_socket sock)
{

	while (!exit_loop)
	{
		auto [NDI_audio_frame, data, dataSize] = FrameRecever::ReceveAudioFrame(sock);

		NDI_audio_frame.p_data = data;

		//printf("Submitted audio frame with timestamp: %" PRId64 "\n", NDI_audio_frame.timestamp);

		NDIlib_send_send_audio_v2(pNDI_send, &NDI_audio_frame);
	}
}

int main()
{
	sockpp::socket_initializer sockInit;

	int pedal = 1;
	sockpp::tcp_acceptor acceptor_video(pedal);

	sockpp::tcp_acceptor acceptor_video2(1339);
	sockpp::tcp_acceptor acceptor_audio(1338);

	LOG("Waiting for video on port 1337");
	LOG("Waiting for audio on port 1338");

	while (true) 
	{
		sockpp::inet_address peer;
		sockpp::tcp_socket video_socket = acceptor_video.accept(&peer);
		sockpp::tcp_socket audio_socket = acceptor_audio.accept(&peer);

		LOG("Incoming connection from " << peer);

		NDIlib_send_create_t NDI_send_create_desc;
		NDI_send_create_desc.p_ndi_name = "zzzz";
		//NDI_send_create_desc.clock_video = true;

		pNDI_send = NDIlib_send_create(&NDI_send_create_desc);

		std::thread video_thread(VideoHandler, std::move(video_socket));
		video_thread.detach();


		std::thread audio_thread(AudioHandler, std::move(audio_socket));
		audio_thread.detach();
	}
}