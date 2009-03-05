#ifndef SIMOUT_H_INCLUDED
#define SIMOUT_H_INCLUDED

#include <ncurses.h>

static WINDOW* mainwin;
static WINDOW* screen;

void screen_init(void);
void screen_end(void);
void update_server(int stid, double utilized, int served);

#endif // SIMOUT_H_INCLUDED
