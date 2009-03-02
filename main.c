#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>
#include "customer.h"

#define DEBUG

//Default settings and macros
#define SERVER_MAX        5
#define DEFAULT_LAMBDA    3.0
#define DEFAULT_MU        4.0
#define DEFAULT_CUSTOMERS 1000
#define DEFAULT_SERVERS   2
#define DEFAULT_QMODE     FIFO
#define DEFAULT_SEED      0

//Simulation queues
cqueue* live; //Queue of waiting customers
cqueue* dead; //Queue of served customers for statistics

//Flags and locks
pthread_mutex_t lock;         //Thread locking mutex
int terminate_simulation = 0; //Kill flag to kill the simulation
int genisis_complete     = 0; //Flag to signal no more customers being created

//Structures and typedefs
typedef struct _genisis_data {
    int    customers;
    double lambda;
    double mu;
    double rseed;
} genisis_data;

//Prototypes and inline functions
inline double rexp(double l) {return -log(1.0-drand48())/l;}
void* genisis(void*);
void* service(void*);
void* output(void);
void* watchman(void);
void  psleep(double interval);

int main(int argc, char** argv)
{
    genisis_data gdata;
    cqmode mode      = DEFAULT_QMODE;
    int    servers   = DEFAULT_SERVERS;
    gdata.rseed      = DEFAULT_SEED;
    gdata.customers  = DEFAULT_CUSTOMERS;
    gdata.lambda     = DEFAULT_LAMBDA;
    gdata.mu         = DEFAULT_MU;

    ////////////////////////////////////////////////////////////////////////
    // Parse command line parameters
    int i = 0;
    for(i = 1; i < argc; i++) {
        switch(argv[i][1]) {
            case 'N':
                if(i+1 >= argc) {
                    printf("Incomplete argument 'N'\n");
                    exit(-1);
                }
                servers = atoi(argv[++i]);
                break;
            case 'L':
                if(i+1 >= argc) {
                    printf("Incomplete argument 'L'\n");
                    exit(-1);
                }
                gdata.lambda = (double)atof(argv[++i]);
                break;
            case 'M':
                if(i+1 >= argc) {
                    printf("Incomplete argument 'M'\n");
                    exit(-1);
                }
                gdata.mu = (double)atof(argv[++i]);
                break;
            case 'T':
                if(i+1 >= argc) {
                    printf("Incomplete argument 'T'\n");
                    exit(-1);
                }
                gdata.customers = atoi(argv[++i]);
                break;
            case 'R':
                if(i+1 >= argc) {
                    printf("Incomplete argument 'R'\n");
                    exit(-1);
                }
                gdata.rseed = (double)atof(argv[++i]);
                break;
            case 'S':
                if(i+1 >= argc) {
                    printf("Incomplete argument 'S'\n");
                    exit(-1);
                }
                if(atoi(argv[++i]))
                    mode = SJF;
                else
                    mode = FIFO;
                break;
            default:
                printf("Invalid argument '%s'\n",argv[i]);
                exit(-1);
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////
    //Enforce restrictions
    if(servers > SERVER_MAX || servers <= 0) {
        printf("The number of servers is restricted between 1 and %d\n",SERVER_MAX);
        exit(-1);
    } else if(gdata.mu*servers < gdata.lambda) {
        printf("The product of mu and the number of servers must be greater than lambda\n");
        exit(-1);
    }
    if(!gdata.rseed) gdata.rseed = time(NULL);

    ////////////////////////////////////////////////////////////////////////
    //Configuration debug statements
    #ifdef DEBUG
    printf("Servers\t\t: %d\n",servers);
    printf("Mu\t\t: %lf\n",gdata.mu);
    printf("Lambda\t\t: %lf\n",gdata.lambda);
    printf("Seed\t\t: %lf\n",gdata.rseed);
    printf("Iterations\t: %d\n",gdata.customers);
    if(mode == FIFO)
        printf("Mode\t\t: FIFO\n");
    else
        printf("Mode\t\t: SJF\n");
    #endif

    ////////////////////////////////////////////////////////////////////////
    //Setup queues
    live = new_cqueue(mode);
    dead = new_cqueue(FIFO);
    if(!live || !dead) {
        printf("Error: queue memmory allocation failed\n");
        if(live) destroy_cqueue(live);
        if(dead) destroy_cqueue(dead);
        exit(-1);
    }

    /////////////////////////////////////////////////////////////////////////
    //Setup threads
    pthread_t      service_t[SERVER_MAX];
    pthread_t      genisis_t;
    pthread_t      output_t;
    pthread_t      watchman_t;
    pthread_attr_t attributes;
    int            status, terror;

    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init(&lock,NULL);

    if((terror = pthread_create(&genisis_t,&attributes,(void*)genisis,(void*)&gdata))) {
        printf("Error creating gensis thread (Code:%d)\n",terror);
        destroy_cqueue(live);
        destroy_cqueue(dead);
        exit(-1);
    }
    for(i = 0; i < servers; i++) {
        if((terror = pthread_create(&service_t[i],&attributes,(void*)service,(void*)i))) {
            printf("Error creating server thread #%d (Code:%d)\n",i,terror);
            destroy_cqueue(live);
            destroy_cqueue(dead);
            exit(-1);
        }
    }
    for(i = 0; i < servers; i++) {
        if((terror = pthread_join(service_t[i], (void **)&status))) {
            printf("Error joing service thread #%d (Code:%d)\n",i,terror);
            exit(-1);
        }
    }
    if((terror = pthread_join(genisis_t, (void **)&status))) {
        printf("Error joing genisis thread (Code:%d)\n",terror);
        exit(-1);
    }

    /////////////////////////////////////////////////////////////////////////
    //Clean up
    pthread_attr_destroy(&attributes);
    pthread_mutex_destroy(&lock);
    destroy_cqueue(live);
    destroy_cqueue(dead);
    pthread_exit(NULL);
    return 0;
}

void psleep(double interval) {
    struct timespec t;
    t.tv_sec  = (int)floor(interval);
    t.tv_nsec = (interval-t.tv_sec) * 1000000000L;
    nanosleep(&t,NULL);
}

void* genisis(void* targ) {
    genisis_data* gdata = (genisis_data*)targ;
    int generated = 0;
    customer* c = NULL;
    timeval birthday;
    srand48(gdata->rseed);
    while(generated < gdata->customers) {
        #ifdef DEBUG
        printf("Generating(%d)...\n",generated);
        #endif
        gettimeofday(&birthday,NULL);
        pthread_mutex_lock(&lock);
        if(terminate_simulation) {
            pthread_mutex_unlock(&lock);
            pthread_exit(NULL);
        }
        c = new_customer(rexp(gdata->mu),birthday);
        encqueue(live,c);
        pthread_mutex_unlock(&lock);
        generated++;
        psleep(rexp(gdata->lambda));
    }
    pthread_mutex_lock(&lock);
    genisis_complete = 1;
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
    return NULL;
}

void* service(void* targ) {
    int stid = (int)targ;
    customer* c = NULL;
    timeval deathday;
    while(1) {
        pthread_mutex_lock(&lock);
        if(terminate_simulation) {
            pthread_mutex_unlock(&lock);
            pthread_exit(NULL);
        }
        if(live->count == 0 && genisis_complete) {
            pthread_mutex_unlock(&lock);
            pthread_exit(NULL);
        }
        c = decqueue(live);
        pthread_mutex_unlock(&lock);

        if(c == NULL) {
            #ifdef DEBUG
            printf("Server #%d idling...\n",stid);
            #endif
            psleep(0.01);
            continue;
        }

        #ifdef DEBUG
        printf("Server #%d servicing...\n",stid);
        #endif
        psleep(c->job);
        gettimeofday(&deathday,NULL);
        c->died = deathday;
        pthread_mutex_lock(&lock);
        encqueue(dead, c);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}
