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
    int y = 8 + (stid/2) + (3*stid/2);
    int x = (stid%2)*24;
    mvwprintw(screen,y+0,1+x,"Server #%d Statistics",stid+1);
    mvwprintw(screen,y+1,1+x,"Served   : %d",served);
    mvwprintw(screen,y+2,1+x,"Utilized : %3.2lf%%",utilized);
    mvwprintw(screen,y+3,1+x,"-----------------------");
    wrefresh(screen);
    refresh();
}

void update_queue_stats(double average, double sigma) {
    mvwprintw(screen,1,1,"Queue Length Statistics");
    mvwprintw(screen,2,1,"Average  : %.2lf", average);
    mvwprintw(screen,3,1,"Sigma    : %.2lf", sigma);
    mvwprintw(screen,4,1,"-----------------------");
    wrefresh(screen);
    refresh();
}

void update_wait_stats(double average, double sigma) {
    mvwprintw(screen,5,1,"Waiting Time Statistics");
    mvwprintw(screen,6,1,"Average  : %.2lf", average);
    mvwprintw(screen,7,1,"Sigma    : %.2lf", sigma);
    mvwprintw(screen,8,1,"-----------------------");
    wrefresh(screen);
    refresh();
}

void update_progress(double time, double complete){;}
