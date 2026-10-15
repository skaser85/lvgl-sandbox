#include "lvgl/lvgl.h"
#include <SDL2/SDL.h>
#include <unistd.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

int main(int argc, char **argv) {
    lv_init();

    // 1. Initialize the virtual display window canvas
    lv_display_t * disp = lv_sdl_window_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!disp) return 1;

    // 2. Create the scrolling menu widget matching your status types
    lv_obj_t * roller = lv_roller_create(lv_screen_active());
    lv_roller_set_options(roller,
        "Available\n"
        "Busy\n"
        "On a Call\n"
        "In a Meeting",
        LV_ROLLER_MODE_NORMAL);

    lv_obj_set_width(roller, 120);
    lv_roller_set_visible_row_count(roller, 3); 
    lv_obj_center(roller);

    // 3. Turn on the keyboard/encoder input driver simulation layer
    // (This tells the sandbox to convert keyboard events into navigation clicks)
    lv_indev_t * indev = lv_sdl_keyboard_create();
    
    // 4. Create an interaction group and bind the scrolling menu to it
    lv_group_t * g = lv_group_create();
    lv_group_add_obj(g, roller);
    lv_indev_set_group(indev, g); // Links the keyboard controls to this menu item

    // Main execution loop
    while (1) {
        lv_timer_handler();
        usleep(5000); 

        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                break;
            }
            
            // Catch when the user presses a key down to simulate clicks
            if (event.type == SDL_KEYDOWN) {
                uint16_t selected = lv_roller_get_selected(roller);
                
                if (event.key.keysym.sym == SDLK_RETURN) {
                    // Simulates pressing the knob down
                    printf("[CLICK] Selected Status Index: %d\n", selected);
                    // (When you are home, this click will trigger the ESP-NOW broadcast!)
                } else if (event.key.keysym.sym == SDLK_DOWN) {
                  uint16_t selected = lv_roller_get_selected(roller);
                  if (selected < 4) selected += 1;
                  lv_roller_set_selected(roller, selected, LV_ANIM_ON); 
                } else if (event.key.keysym.sym == SDLK_UP) {
                  uint16_t selected = lv_roller_get_selected(roller);
                  if (selected > 0) selected -= 1;
                  lv_roller_set_selected(roller, selected, LV_ANIM_ON); 
                }
            }
        }
    }

    return 0;
}

