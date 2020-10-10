#include "Transcoder.h"

std::atomic<NDIlib_send_instance_t> pNDI_send;

std::atomic<bool> shouldStart(false);

static std::atomic<bool> exit_loop(false);
static void sigint_handler(int)
{
	exit_loop = true;
}


void VideoHandler(sockpp::tcp_socket sock)
{
	TranscoderSettings settings;
	settings.bitrate = 2500;
	settings.codecId = AV_CODEC_ID_H264;
	settings.pix_fmt = AV_PIX_FMT_NV12;
	settings.isEncoder = false;

	Transcoder* transcoder = new Transcoder(settings);

	shouldStart = true;

	while (!exit_loop)
	{
		VideoFrame frame;

		if (sock.read_n((void*)&frame, sizeof(VideoFrame)) == -1)
		{
			std::cout << "DetailsRead: " << sock.last_error_str() << "\n";
		}

		NDIlib_video_frame_v2_t NDI_video_frame;


		if (frame.dataSize != 0)
		{
			NDI_video_frame = frame.videoFrame;

			char* dataBuffer = (char*)malloc(frame.videoFrame.xres * frame.videoFrame.yres * 2);

			if (sock.read_n((void*)dataBuffer, frame.dataSize) == -1)
			{
				std::cout << "DetailsRead: " << sock.last_error_str() << "\n";
			}
			
			auto [decodedSize, decodedData] = transcoder->Decode((uint8_t*)dataBuffer, frame.dataSize);

			if (decodedSize != 0)
			{
				NDI_video_frame.p_data = decodedData;
			}
			else
			{
				NDI_video_frame = NDIlib_video_frame_v2_t(1, 1);
				NDI_video_frame.p_data = (uint8_t*)malloc(2);
				memset(NDI_video_frame.p_data, 0, 2);
			}
			free(dataBuffer);
		}
		else
		{
			NDI_video_frame = NDIlib_video_frame_v2_t(1, 1);
			NDI_video_frame.p_data = (uint8_t*)malloc(2);
			memset(NDI_video_frame.p_data, 0, 2);
		}

		NDIlib_send_send_video_v2(pNDI_send, &NDI_video_frame);
	}
	//NDIlib_send_destroy(pNDI_send);
}

void AudioHandler(sockpp::tcp_socket sock)
{
	while (shouldStart == false)
	{
		printf("Waiting for video thread!\n");
	}

	ssize_t n = 1;

	AUDIO_FRAME* frame = (AUDIO_FRAME*)malloc(sizeof(AUDIO_FRAME));
	while ((n = sock.read_n((void*)frame, sizeof(AUDIO_FRAME))) > 0)
	{
		NDIlib_audio_frame_v2_t NDI_audio_frame;
		NDI_audio_frame = frame->audioFrame;
		NDI_audio_frame.p_data = (float*)frame->data;

		//printf("Submitted audio frame with timestamp: %" PRId64 "\n", NDI_audio_frame.timestamp);

		NDIlib_send_send_audio_v2(pNDI_send, &NDI_audio_frame);
	}

	//NDIlib_send_destroy(pNDI_send);
}

int main()
{
	sockpp::socket_initializer sockInit;
	sockpp::tcp_acceptor acceptor_video(1337);
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