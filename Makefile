CXX=g++
LD=g++

W_FLAGS=-pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef -Werror -Wno-unused -Wconversion

CXX_FLAGS=-std=c++20 -fopenmp -D_GLIBCXX_PARALLEL -Iinclude $(W_FLAGS)

L_FLAGS=-L/usr/lib/x86_64-linux-gnu -lglfw -lvulkan -fopenmp -flto

DEBUG=-g -Og
RELEASE=-DNDEBUG -O3 -flto -fno-signed-zeros -fno-trapping-math -frename-registers -funroll-loops -mavx -march=native

build/debug/vulkan-tutorial: build/debug/main.o
	$(LD) -o $@ $^ $(L_FLAGS)
build/debug/main.o: src/main.cc
	$(CXX) $(CXX_FLAGS) $(DEBUG) -c -o $@ $<

build/release/vulkan-tutorial: build/release/main.o
	$(LD) -o $@ $^ $(L_FLAGS)
build/release/main.o: src/main.cc
	$(CXX) $(CXX_FLAGS) $(RELEASE) -c -o $@ $<

debug: build/debug/vulkan-tutorial
	__GL_SYNC_TO_VBLANK=0 ./$<
release: build/release/vulkan-tutorial
	__GL_SYNC_TO_VBLANK=0 ./$<

clean:
	rm -rf build/debug/*.o
	rm -rf build/release/*.o
	rm -rf build/debug/vulkan-tutorial
	rm -rf build/release/vulkan-tutorial

.DEFAULT: vulkan-tutorial
.PHONY: exe clean
