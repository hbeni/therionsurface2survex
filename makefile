CC = g++
CFLAGS=-v
SRC = therionsurface2survex


all:
	$(CC) -o therionsurface2survex therionsurface2survex.cxx $(CFLAGS)
