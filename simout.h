#ifndef SIMOUT_H_INCLUDED
#define SIMOUT_H_INCLUDED

#include <curses.h>

static WINDOW* mainwin;
static WINDOW* screen;

void screen_init(void);
void screen_end(void);

#endif // SIMOUT_H_INCLUDED
