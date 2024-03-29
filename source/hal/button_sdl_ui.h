#ifndef BUTTON_UI_H_
#define BUTTON_UI_H_

#include "button_coords.h"

/* This file defines functions/vars in button_sim.c or init_sdl_sim.c meant to allow these
 * two modules to communicate information back and forth.
 */
/* Callbacks in button_sim.c called from init_sdl_sim.c */
extern int time_to_quit;
extern int key_press_cb(SDL_Keysym *keysym);
extern int key_release_cb(SDL_Keysym *keysym);
extern int mouse_button_down_cb(SDL_MouseButtonEvent *event, struct button_coord_list *bcl);
extern int mouse_button_up_cb(SDL_MouseButtonEvent *event);
extern int mouse_scroll_cb(SDL_MouseWheelEvent *event, struct button_coord_list *bcl);
extern void handle_window_event(SDL_Window *window, SDL_Event event);
extern int joystick_event_cb(__attribute__((unused)) SDL_Window *window, SDL_Event event);
extern int quit_confirm_active;
extern int quit_confirmed(int x, int y);
extern void simulator_zoom_ui(float factor);

/* This is used for showing UI inputs on the badge simulator on screen.
 * They are counters that count down to zero when a button is pressed.
 * This is so the UI has time to actually display the button presses
 * instead of the sim completely missing that a button was pressed at
 * all.
 */
struct sim_button_status {
	int button_a;
	int button_b;
	int left_rotary_button;
	int right_rotary_button;
	int dpad_up;
	int dpad_down;
	int dpad_right;
	int dpad_left;
};

struct sim_button_status get_sim_button_status(void);
void sim_button_status_countdown(void);
int sim_get_rotary_angle(int which_rotary);

/* Functions in init_sdl_sim.c called from button_sim.c */
extern void toggle_fullscreen_mode(void);

#endif
