all: build
	export CXX=clang++
	ninja -C build -j $(shell cat /proc/cpuinfo | grep -c processor)

clean: build
	ninja -C build clean

test: build
	export CXX=g++
	ninja -C build test -j $(shell cat /proc/cpuinfo | grep -c processor)

bench: build
	ninja -C build bench

build:
	mkdir -p build
	cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja -S . -B build

tidy: build
	ninja -C build tidy

.PHONY: bench build clean test tidy all
