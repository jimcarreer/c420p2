#include "simout.h"

void screen_init(char* m) {
    mainwin = initscr();
    noecho();
    cbreak();
    refresh();
    wrefresh(mainwin);
    screen = newwin(22, 50, 1, 1);
    box(screen, ACS_VLINE, ACS_HLINE);
    curs_set(0);
    mvwprintw(screen,0,1,"Queue Simulation Statistics (%s)", m);
}

void screen_end(void) {
    endwin();
}

void update_server(int stid, double u, int s) {
    curs_set(0);
    int y = 8 - (stid%2)*1 + (stid/2) + (3*stid/2);
    int x = (stid%2)*24 + 1;
    mvwprintw(screen,y+0,x,"Server #%d Statistics",stid+1);
    mvwprintw(screen,y+1,x,"Served   : %d",s);
    mvwprintw(screen,y+2,x,"Utilized : %3.2lf%%",u);
    mvwprintw(screen,y+3,x,"-----------------------");
    wrefresh(screen);
    refresh();
}

void update_queue_stats(double a, double s) {
    mvwprintw(screen,4,1,"Queue Length Statistics");
    mvwprintw(screen,5,1,"Average  : %.2lf", a);
    mvwprintw(screen,6,1,"Sigma    : %.2lf", s);
    mvwprintw(screen,7,1,"-----------------------");
    wrefresh(screen);
    refresh();
}

void update_wait_stats(double a, double s) {
    mvwprintw(screen,4,25,"Waiting Time Statistics");
    mvwprintw(screen,5,25,"Average  : %.2lf", a);
    mvwprintw(screen,6,25,"Sigma    : %.2lf", s);
    mvwprintw(screen,7,25,"-----------------------");
    wrefresh(screen);
    refresh();
}

void update_progress(double s, double u, int a, int t) {
    double c = (100)*a/(double)t;
    mvwprintw(screen,1,1, "Completed : %3.2lf%%", c);
    mvwprintw(screen,2,1, "Served    : %d", a);
    mvwprintw(screen,1,25,"Time Elapsed : %.0lfs", s);
    mvwprintw(screen,2,25,"Utilized     : %3.2lf%%", u);
    wrefresh(screen);
    refresh();
}

void wait_for_user() {
    mvwprintw(screen,20,1,"Press any key to quit");
    wrefresh(screen);
    refresh();
    getch();
    return;
}
