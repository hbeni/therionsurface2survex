CC = g++
CFLAGS=-v -O3
SRC = therionsurface2survex
VERSION:=$(shell grep -o 'VERSION.\+"[0-9.]\+"' therionsurface2survex.cxx | cut -f 4 -d' ' | sed 's/"//g')

all: linux win64

linux:
	$(CC) -o therionsurface2survex therionsurface2survex.cxx $(CFLAGS)

# Windows 64bit build
#   apt-get install mingw-w64
win64:
	x86_64-w64-mingw32-g++ -o therionsurface2survex.exe therionsurface2survex.cxx -static-libgcc -static-libstdc++ $(CFLAGS)

test: linux
	therionsurface2survex -h
	therionsurface2survex example/surface.th -t
	ls -alh example/surface.th.th

clean:
	rm -f example/surface.th.th

release: clean all
	@echo building release: ${VERSION}
	mkdir therionsurface2survex-${VERSION}
	cp -r example/ LICENSE README.md therionsurface2survex-${VERSION}/
	cp therionsurface2survex therionsurface2survex.exe therionsurface2survex-${VERSION}/
	tar -czf therionsurface2survex-${VERSION}.tar.gz therionsurface2survex-${VERSION}/
	rm -rf therionsurface2survex-${VERSION}/
	ls -alh therionsurface2survex-${VERSION}.tar.gz
	tar -tzf therionsurface2survex-${VERSION}.tar.gz


showVer:
	@echo "VERSION:$(VERSION)"
