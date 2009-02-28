#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "customer.h"

extern customer* decqueue(cqueue* queue);
extern void      encqueue(cqueue* queue, customer* customer);
extern cqueue*   new_cqueue(mode);
extern customer* new_customer(double job, timeval born, timeval died);
extern void      destroy_cqueue(cqueue* queue);
extern void      destroy_customer(customer* condemed);
extern void      print_cqueue_trace(cqueue queue);

int main()
{
    timeval   t;
    int       i = 0;
    double    j = 0;
    cqueue*   q = new_cqueue(SJF);
    customer* c = NULL;
    srand48((long)time(NULL));

    printf("Starting...\n");
    for(i = 0; i < 10; i++) {
        j = drand48();
        c = new_customer(j,t,t);
        encqueue(&q,c);
    }
    print_cqueue_trace(*q);
    destroy_cqueue(q);
    print_cqueue_trace(*q);
    printf("Done...\n");
    q = NULL;
    return 0;
}

