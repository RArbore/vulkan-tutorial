CXX=g++
LD=g++

W_FLAGS=-pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef -Werror -Wno-unused -Wconversion

CXX_FLAGS=-std=c++20 -O3 -flto -fno-signed-zeros -fno-trapping-math -frename-registers -funroll-loops -fopenmp -D_GLIBCXX_PARALLEL -mavx -march=native -Iinclude $(W_FLAGS)

L_FLAGS=-L/usr/lib/x86_64-linux-gnu -lglfw -lvulkan -fopenmp -flto

vulkan-tutorial: build/main.o
	$(LD) -o $@ $^ $(L_FLAGS)
build/main.o: src/main.cc
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

exe: vulkan-tutorial
	__GL_SYNC_TO_VBLANK=0 ./$<

clean:
	rm -rf build/*.o
	rm -rf vulkan-tutorial

.DEFAULT: vulkan-tutorial
.PHONY: exe clean
