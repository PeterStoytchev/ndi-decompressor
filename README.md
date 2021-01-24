# ndi-server
The server (and for now decompressor) for my ndi-compressor project. This is what you would connect to.

For now, this only works on Windows, because the NDI library is Windows only.
Later on, when the server is standalone, it will be cross-platform.

For now, run this on the device you would like to receive the NDI stream on. Once it starts receiving a signal, from a ndi-compressor, it will create a NDI source from it, with the name specified in the config. 

Keep in mind that this is a hobby project created by a novice. Do not expect miracles. That said, I will try and make it as good as possible.

I will keep the master branch stable, and will push anything new to dev (as is customary).

# How to compile and install.
1. Clone the repo.
2. Open in Visual Studio (developed on VS2019, it will probably work on older versions, as long as C++17 is supported)
3. Set the configuration to Debug or Release, depending on what you want, and arch to x64. x64 is what the included libs are compiled for. If you want x86, recompile the libs yourself, replace them and set the config to x86.
4. Build the solution.
5. Copy the config.yaml file next to the newly build .exe and change it as needed.
6. From the redist directory, copy all the files next to the new .exe
7. Run the server.

# Updates
UPDATE 1: Multi-threading the various stages ended up being not a good idea, impact was negative in my testing. Currently the system is back to a single thread for video and another one for audio. I may revisit the idea at some point. Did some testing over WAN and network performance is currently a big problem. This is my main issue to solve. It works well over LAN (duh, so does NDI) as well as Wi-Fi as long as the bitrate is kept in check. I also added support for the Optik profiler. By default it is enabled, if you want to disable it, remove the "#define __PROFILE" in Profiler.h and recompile.

# TODO
1. Build system support.
2. Build most of the dependencies with the build system, instead of including precompiled binaries (aside from NDI lib, which isn't open source)
3. Some sort of compression for audio. Currently thinking ZLib or LZ4, but could also be fed into ffmpeg.

# Long-term TODO
Decouple the server and decompressor. The idea is to have a server hosted in the cloud (wow really?) and have it act as an amplifier of sorts for the signal it receves from an encoder. This will happen once the networking and compression/decompression work smoothly.
