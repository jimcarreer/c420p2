#include "simout.h"

void screen_init(void) {
    mainwin = initscr();
    noecho();
    cbreak();
    nodelay(mainwin, TRUE);
    refresh();
    wrefresh(mainwin);
    screen = newwin(50, 30, 1, 1);
    box(screen, ACS_VLINE, ACS_HLINE);
}

void screen_end(void) {
    endwin();
}

void update_server(int stid, double utilized, int served) {
    curs_set(0);
    int y = 0 + stid + (3*stid);
    mvwprintw(screen,y+0,6,"Server #%d Statistics",stid+1);
    mvwprintw(screen,y+1,6,"Served   : %d",served);
    mvwprintw(screen,y+2,6,"Utilized : %3.2lf",utilized);
    wrefresh(screen);
    refresh();
}
