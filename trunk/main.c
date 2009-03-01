#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>
#include "customer.h"

//Constants
#define SERVER_MAX 5;

//Simulation defaults
double  lambda = 3.0;  //Arrival time distribution parameter
double  mu     = 4.0;  //Service time distribution parameter
int     cnum   = 1000; //Number of customers for simulation
int     snum   = 1;    //Number of servers for simulation
double  seed   = 0;    //Random number generator seed
cqmode  mode   = FIFO; //Operational mode
cqueue* live;          //Queue of waiting customers
cqueue* dead;          //Queue of served customers for statistics
int     tkill  = 0;    //Kill flag to kill the simulation

//Mutex
pthread_mutex_t lock;

//Prototypes
inline double rexp(double l) {return -log(1.0-drand48())/l;}
void* genisis(void);
void* service(void);
void* output(void);
void* watchman(void);
void  psleep(double interval);

int main()
{
    ////////////////////////////////////////////////////////////////////////
    //Setup queues
    live = new_cqueue(mode);
    dead = new_cqueue(FIFO);
    if(!live || !dead) {
        printf("Error list memmory allocation failed\n");
        if(live) destroy_cqueue(live);
        if(dead) destroy_cqueue(dead);
        exit(-1);
    }
    /////////////////////////////////////////////////////////////////////////
    //Setup threads
    pthread_t     service_t[SERVER_MAX];
    pthread_t     genisis_t;
    pthread_t     output_t;
    pthread_t     watchman_t;
    pthread_att_t attributes;
    int           terror = 0;

    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init(&lock,NULL);

    int i = 0;
    for(i = 0; i < snum; i++) {
        if(terror = pthread_create(&genisis_t,&attributes,genisis,NULL)) {
            printf("Error creating server thread #%d (Code:%d)\n",i,terror);
            destroy_cqueue(live);
            destory_cqueue(dead);
            exit(-1);
        }
    }
    if(terror = pthread_create(&genisis_t,&attributes,genisis,NULL)) {
        printf("Error creating gensis thread (Code:%d)\n",terror);
        destroy_cqueue(live);
        destory_cqueue(dead);
        exit(-1);
    } /*else if(terror = pthread_create(&output_t,&attributes,output,NULL)) {
        destroy_cqueue(live);
        destory_cqueue(dead);
        printf("Error creating output thread (Code:%d)\n",terror);
        exit(-1);
    }*/
    pthread_attr_destroy(&attributes);
    /////////////////////////////////////////////////////////////////////////

    destroy_cqueue(live);
    destroy_cqueue(dead);
    return 0;
}

void psleep(double interval) {
    struct timespec t;
    t.tv_sec  = (int)floor(interval);
    t.tv_nsec = (interval-t.tv_sec) * 1000000000L;
    nanosleep(&t,NULL);
}

void* genisis() {
    int       generated = 0;
    timeval   birthday;
    customer* customer  = NULL;
    while(generated < cnum) {
        if(tkill) pthread_exit(NULL);

        gettimeofday(&birthday,NULL);

        pthread_mutex_lock(&lock);
        customer = new_customer(rexp(mu),birthday);
        encqueue(live,customer);
        pthread_mutex_unlock(&lock);

        generated++;
        psleep(rexp(lambda));
    }
    pthread_exit(NULL);
}

void* service() {
    int       serviced = 0;
    timeval   deathday;
    customer* customer = NULL;
    while(serviced < cnum) {
        if(tkill) pthread_exit(NULL);

        pthread_mutex_lock(&lock);
        customer = decqueue(live);
        pthread_mutex_unlock(&lock);

        //Idling
        if(customer == NULL) {
            psleep(0.02);
            continue;
        }

        //Servicing
        psleep(customer->job);
        gettimeofday(&deathday,NULL);
        customer->died = deathday;

        pthread_mutex_lock(&lock);
        encqueue(dead,customer);
        pthread_mutex_unlock(&lock);

        serviced++;
    }
    pthread_exit(NULL);
}
