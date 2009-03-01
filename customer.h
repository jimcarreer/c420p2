#ifndef CUSTOMER_H_INCLUDED
#define CUSTOMER_H_INCLUDED

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

//Different insert modes
enum _cqmode {FIFO = 0, SJF = 1};

//Misc Typedefs
typedef enum   _cqmode cqmode;
typedef struct timeval timeval;

//Structure for holding customer data
typedef struct _customer {
    struct _customer* next; //Next customer in queue
    struct _customer* prev; //Previous customer in queue
    struct timeval    born; //Time customer enters system
    struct timeval    died; //Time customer leaves system
    double            job;  //Customer's job time (microseconds)
} customer;

//Structure for customer queue
typedef struct _cqueue {
    struct _customer*  head; //First customer of queue
    struct _customer*  tail; //Last customer of queue
    int               count; //Number of customers enqueued
    enum   _cqmode     mode; //Insertion mode
} cqueue;

customer* decqueue(cqueue* queue);
void      encqueue(cqueue* queue, customer* customer);
cqueue*   new_cqueue(cqmode mode);
customer* new_customer(double job, timeval born);
void      destroy_cqueue(cqueue* queue);
void      destroy_customer(customer* condemed);
void      print_cqueue_trace(cqueue queue);

#endif // CUSTOMER_H_INCLUDED
