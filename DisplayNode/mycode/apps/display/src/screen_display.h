#ifndef SCREEN_DISPLAY_H
#define SCREEN_DISPLAY_H

#include "display_mode.h"
#include "orientation_enum.h"

#define MAX_ANIMATION_FRAMES 6

#define SCREEN_THREAD_STACK_SIZE 1024
#define SCREEN_THREAD_PRIORITY 5

int screen_display_init(void);

void screen_display_save_image(const void *buffer);

void screen_display_save_image_reverse(const void *buffer);

int screen_display_save_animation_frame(int frame_num, const void *buffer);

int screen_display_save_animation_frame_reverse(int frame_num, const void *buffer);

int screen_display_set_mode(enum DisplayMode mode);

int screen_display_set_animation_frame(int frame);

int screen_display_set_orientation(enum Orientation orientation);

#endif
