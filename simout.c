#include "simout.h"

void screen_init(void) {
    mainwin = initscr
    noecho();
    cbreak();
    nodelay(mainwin, TRUE);
    refesh();
    wrefresh(mainwin);
    screen = newwin(13, 27, 1, 1);
    box(screen, ACS_VLINE, ACS_HLINE);
}

void screen_end(void) {
    endwin();
}
