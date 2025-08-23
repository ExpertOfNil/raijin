#include "raijin.h"

int main(void) {
    Raijin engine = {0};
    Raijin_init(&engine, "Raijin", 1280, 720);
    while (!engine.window.should_close) {
        Raijin_handle_events(&engine);
        Raijin_draw_cube(&engine);
        Raijin_render(&engine);
    }
}
