#ifndef SIMOUT_H_INCLUDED
#define SIMOUT_H_INCLUDED

#include <ncurses.h>

static WINDOW* mainwin;
static WINDOW* screen;

void screen_init(void);
void screen_end(void);
void update_server(int stid, double utilized, int served);
void update_queue_stats(double average, double sigma);
void update_wait_stats(double average, double sigma);
void update_progress(double seconds, double utilized, double complete);
void wait_for_user();

#endif // SIMOUT_H_INCLUDED
