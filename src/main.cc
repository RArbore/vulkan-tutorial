#include <chrono>

#include "graphics.h"

__attribute__((always_inline))
inline unsigned long long micro_sec() {
    return static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
}

int main() {
    Graphics graphics;
    
    float dt = 0.0f;
    unsigned long long before = 0, after = 0;
    while (!graphics.should_close()) {
	before = micro_sec();
	graphics.render_tick();
	after = micro_sec();
	dt = static_cast<float>(after - before) / 1000000.0f;
	std::cout << "FPS: " << 1. / dt << '\n';
    }
    
    return 0;
}
