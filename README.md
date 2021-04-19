# ndi-decompressor
The server for my ndi-compressor project. This is what you would connect to.

Run this on the device you would like to receive the NDI stream on. Once it starts receiving a signal, from a ndi-compressor, it will create a NDI source from it, with the name specified in the config. 

Keep in mind that this is a hobby project created by a novice. Do not expect miracles. That said, I will try and make it as good as possible.

I will keep the master branch stable, and will push anything new to dev (as is customary).

# How to compile and install.
1. Install premake5, if you don't have it installed already.
2. Clone the repo.
3. Run "premake5 vs2019" to generate VS2019 project files. If you want to build with something else, refer to the premake docs for that. (developed on VS2019, it will probably work on older versions, as long as C++17 is supported)
4. Build it. (Note: If you want to build for anything else, aside from Windows x86_64, you will need to recompile the libraries and replace them in the vendor folder)
5. Copy the config.yaml file next to the newly build .exe and change it as needed.
6. From the redist directory, copy all the files next to the new .exe
7. Run the server.

# Updates
UPDATE 1: Multi-threading the various stages ended up being not a good idea, impact was negative in my testing. Currently the system is back to a single thread for video and another one for audio. I may revisit the idea at some point. Did some testing over WAN and network performance is currently a big problem. This is my main issue to solve. It works well over LAN (duh, so does NDI) as well as Wi-Fi as long as the bitrate is kept in check. I also added support for the Optik profiler. By default it is enabled, if you want to disable it, remove the "#define __PROFILE" in Profiler.h and recompile.

UPDATE 2: Apperantly, I didn't know what I was doing when I last tried to multi-thread the compressor. So, yea. Multi-threading is back, but in a diffirent form. The biggest change, compared to before, is frame batching, which is currently baked in to 30 video and 24 audio frames per batch.

# TODO
1. Some sort of compression for audio. Currently thinking ZLib or LZ4, but could also be fed into ffmpeg.
