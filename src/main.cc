#include "graphics.h"

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
