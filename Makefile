all: build
	ninja -C build -j $(shell cat /proc/cpuinfo | grep -c processor)

clean: build
	ninja -C build clean

test: build
	ninja -C build test -j $(shell cat /proc/cpuinfo | grep -c processor)

build:
	mkdir -p build
	cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja -S . -B build

.PHONY: build clean test all
