#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include "board_defs.h"

#ifdef SUPPORT_DISPLAY

// Includes

#include "screen_driver.h"

// Defines

// Statics

extern scr_driver_t     g_lcd;
extern scr_info_t       g_lcd_info;

// Prototypes
void screen_clear(scr_driver_t *lcd, int color);
void init_display();

#endif // SUPPORT_LED

#endif // __DISPLAY_H_