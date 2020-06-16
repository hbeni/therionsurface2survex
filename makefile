CC = g++
CFLAGS=-v
SRC = therionsurface2survex


all: linux win64

linux:
	$(CC) -o therionsurface2survex therionsurface2survex.cxx $(CFLAGS)

# Windows 64bit build
#   apt-get install mingw-w64
win64:
	x86_64-w64-mingw32-g++ -o therionsurface2survex.exe therionsurface2survex.cxx -static-libgcc -static-libstdc++
