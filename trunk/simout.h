#ifndef SIMOUT_H_INCLUDED
#define SIMOUT_H_INCLUDED

#include <ncurses.h>

void screen_init(char* mode);
void screen_end(void);
void update_server(int stid, double utilized, int served);
void update_queue_stats(double average, double sigma);
void update_wait_stats(double average, double sigma);
void update_progress(double seconds, double utilized, int served, int total);
void wait_for_user();

#endif // SIMOUT_H_INCLUDED
