#include "simout.h"

void screen_init(void) {
    mainwin = initscr();
    noecho();
    cbreak();
    nodelay(mainwin, TRUE);
    refresh();
    wrefresh(mainwin);
    screen = newwin(25, 50, 1, 1);
    box(screen, ACS_VLINE, ACS_HLINE);
    curs_set(0);
    mvwprintw(screen,0,1,"Queue Simulation Statistics");
}

void screen_end(void) {
    endwin();
}

void update_server(int stid, double utilized, int served) {
    curs_set(0);
    int y = 6 + stid + (2*stid);
    mvwprintw(screen,y+0,1,"Server #%2d Statistics",stid+1);
    mvwprintw(screen,y+1,1,"Served   : %10d",served);
    mvwprintw(screen,y+2,1,"Utilized : %3.2lf\%",utilized);
    wrefresh(screen);
    refresh();
}
