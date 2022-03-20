#include "graphics.h"

int main() {
    Graphics graphics;
    
    while (!graphics.should_close()) {
	graphics.render_tick();
    }
    
    return 0;
}
