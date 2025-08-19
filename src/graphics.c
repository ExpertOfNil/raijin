#include "graphics.h"
#include "renderer.h"

// Main function
int main(int argc, char* argv[]) {
    GraphicsEngine* engine =
        graphics_engine_create("Graphics Engine", 800, 600);
    if (!engine) {
        return 1;
    }

    graphics_engine_run(engine);
    graphics_engine_destroy(engine);
    return 0;
}
