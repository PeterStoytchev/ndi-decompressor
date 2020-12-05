# ndi-server
The server (and for now decompressor) for my ndi-compressor project. This is what you would connect to.

For now, this only works on Windows, because the NDI library is Windows only.
Later on, when the server is standalone, it will be cross-platform.

# How to compile and install.
1. Clone the repo.
2. Open in Visual Studio (developed on VS2019, it will probably work on older versions, as long as C++17 is supported)
3. Set the configuration to Debug or Release, depending on what you want, and arch to x86. Currently, only x86 is supported.
4. Build the solution.
5. Copy the config.yaml file next to the newly build .exe and change it as needed.
6. From the redist directory, copy all the files next to the new .exe
7. Run the server.

# TODO
1. Profiling support.
2. Seperate the video processing stages (receive, decompress and submit to NDI) onto diffirent threads, in order to reduce latency. 
3. Cmake support.
4. Move to x64.
5. Build most of the dependencies with the build system, instead of including precompiled binaries (aside from NDI lib, which isn't open source).

# Long-term TODO
Decouple the server and decompressor. The idea is to have a server, hosted in the cloud (wow really?) and have it act as an amplifier of sorts for the signal it receves from an encoder. This will happen once the networking and compression/decompression work smoothly.
