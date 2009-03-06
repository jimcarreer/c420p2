#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>
#include <math.h>
#include <ncurses.h>
#include "customer.h"
#include "simout.h"

#define DEBUG

//Default settings and macros
#define SERVER_MAX        5
#define DEFAULT_LAMBDA    3.0
#define DEFAULT_MU        4.0
#define DEFAULT_CUSTOMERS 1000
#define DEFAULT_SERVERS   1
#define DEFAULT_QMODE     FIFO
#define DEFAULT_SEED      0

//Thread input structures
typedef struct _genesis_data {
    double           lambda;          //Arrival time exponential distribution parameter
    double           mu;              //Service time exponential distribution parameter
    double           rseed;           //Seed for random numbers
    cqueue*          live;            //Reference to queue for live (unserviced) customers
    cqueue*          source;          //Reference to queue containing blank customers
    pthread_mutex_t* livelock;        //Reference to mutex to lock live queue
    pthread_mutex_t* displock;        //Reference to mutext to lock display for updating
    sem_t*           customers_left;  //Reference to semaphore of customers left to generate
} genesis_data;

typedef struct _statistics_data {
    int              customers;       //Total number of customers being generated
    cqueue*          live;            //Reference to queue for live (unserviced) customers
    cqueue*          dead;            //Reference to queue for dead (serviced) customers
    pthread_mutex_t* deadlock;        //Reference to mutex to lock dead queue
    pthread_mutex_t* livelock;        //Reference to mutex to lock live queue
    pthread_mutex_t* displock;        //Reference to mutext to lock display for updating
    sem_t*           customers_left;  //Reference to semaphore of customers left to generate
} statistics_data;

typedef struct _service_data {
    int              stid;            //Service thread number
    cqueue*          live;            //Reference to queue for live (unserviced) customers
    cqueue*          dead;            //Reference to queue for dead (serviced) customers
    pthread_mutex_t* deadlock;        //Reference to mutex to lock dead queue
    pthread_mutex_t* livelock;        //Reference to mutex to lock live queue
    pthread_mutex_t* displock;        //Reference to mutex to lock display for updating
    sem_t*           customers_left;  //Reference to semaphore of customers left to generate
} service_data;

//Prototypes and inline functions
inline double rexp(double l) {return -log(1.0-drand48())/l;}
void*   genesis(void*);
void*   service(void*);
void*   statistics(void*);
void    psleep(double interval);
double  time_elapsed(timeval finish, timeval start);

int main(int argc, char** argv)
{
    ////////////////////////////////////////////////////////////////////////
    //Queue variables
    customer*        c;      //Used for allocating memory for customers
    cqueue*          source; //Customer factory
    cqueue*          live;   //Stores unservice customers
    cqueue*          dead;   //Stores serviced customers not yet analyzed
    ////////////////////////////////////////////////////////////////////////
    //Thread parameters
    genesis_data     gensd;
    statistics_data  statd;
    service_data*    servd;
    ////////////////////////////////////////////////////////////////////////
    //Thread related variables
    pthread_mutex_t  deadlock;
    pthread_mutex_t  livelock;
    pthread_mutex_t  displock;

    sem_t            customers_left;

    pthread_t*       service_t;
    pthread_t        genesis_t;
    pthread_t        statistics_t;

    pthread_attr_t   attributes;
    int              terror, i;
    ////////////////////////////////////////////////////////////////////////
    //Simulation related variables
    cqmode mode      = DEFAULT_QMODE;
    int    servers   = DEFAULT_SERVERS;
    double rseed     = DEFAULT_SEED;
    int    customers = DEFAULT_CUSTOMERS;
    double lambda    = DEFAULT_LAMBDA;
    double mu        = DEFAULT_MU;

    ////////////////////////////////////////////////////////////////////////
    // Parse command line parameters
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
                lambda = (double)atof(argv[++i]);
                break;
            case 'M':
                if(i+1 >= argc) {
                    printf("Incomplete argument 'M'\n");
                    exit(-1);
                }
                mu = (double)atof(argv[++i]);
                break;
            case 'T':
                if(i+1 >= argc) {
                    printf("Incomplete argument 'T'\n");
                    exit(-1);
                }
                customers = atoi(argv[++i]);
                break;
            case 'R':
                if(i+1 >= argc) {
                    printf("Incomplete argument 'R'\n");
                    exit(-1);
                }
                rseed = (double)atof(argv[++i]);
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
    } else if(mu*servers < lambda) {
        printf("The product of mu and the number of servers must be greater than lambda\n");
        exit(-1);
    }
    if(!rseed) rseed = time(NULL);

    ////////////////////////////////////////////////////////////////////////
    //Setup queues and allocate all memory we will need before we start
    live   = new_cqueue(mode);
    dead   = new_cqueue(FIFO);
    source = new_cqueue(FIFO);
    if(!live || !dead || !source) {
        printf("Error: queue memmory allocation failed\n");
        exit(-1);
    }
    //Make all the customers and enqueue them into source
    for(i = 0; i < customers; i++) {
        c = new_blank_customer();
        if(c == NULL) {
            printf("Error: queue memmory allocation failed\n");
            exit(-1);
        }
        encqueue(source,c);
    }

    /////////////////////////////////////////////////////////////////////////
    //Multithreading
    //Setup service thread and service thread data
    service_t = (pthread_t*)malloc(servers*sizeof(pthread_t));
    servd     = (service_data*)malloc(servers*sizeof(service_data));
    if(!service_t || !servd) {
        printf("Error: service thread memory allocation failed\n");
        exit(-1);
    }

    //Initialize semaphores
    sem_init(&customers_left, 0, customers);
    //Initialize mutexes
    pthread_mutex_init(&deadlock,NULL);
    pthread_mutex_init(&livelock,NULL);
    pthread_mutex_init(&displock,NULL);
    //Initialize thread attirbutes
    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);

    //Initialize genesis data
    gensd.customers_left = &customers_left;
    gensd.lambda         = lambda;
    gensd.mu             = mu;
    gensd.live           = live;
    gensd.source         = source;
    gensd.livelock       = &livelock;
    gensd.displock       = &displock;
    //Initialize statistics data
    statd.customers_left = &customers_left;
    statd.customers      = customers;
    statd.live           = live;
    statd.dead           = dead;
    statd.livelock       = &livelock;
    statd.deadlock       = &deadlock;
    statd.displock       = &displock;
    //Initialize service data
    for(i = 0; i < servers; i++) {
        servd[i].customers_left = &customers_left;
        servd[i].stid           = i;
        servd[i].live           = live;
        servd[i].dead           = dead;
        servd[i].deadlock       = &deadlock;
        servd[i].livelock       = &livelock;
        servd[i].displock       = &displock;
    }

    //Initialize Display Screen
    screen_init();

    //Start gensis thread
    if((terror = pthread_create(&genesis_t,&attributes,(void*)genesis,(void*)&gensd))) {
        screen_end();
        printf("Error creating gensis thread (Code:%d)\n",terror);
        exit(-1);
    }
    //Start server threads
    for(i = 0; i < servers; i++) {
        if((terror = pthread_create(&service_t[i],&attributes,(void*)service,(void*)(&servd[i])))) {
            screen_end();
            printf("Error creating server thread #%d (Code:%d)\n",i,terror);
            exit(-1);
        }
    }
    //Start statistics threads
    if((terror = pthread_create(&statistics_t,&attributes,(void*)statistics,(void*)&statd))) {
        screen_end();
        printf("Error creating statistics thread (Code:%d)\n",terror);
        exit(-1);
    }
    pthread_attr_destroy(&attributes);

    //Wait for genesis to finish
    if((terror = pthread_join(genesis_t, NULL))) {
        screen_end();
        printf("Error joing genesis thread (Code:%d)\n",terror);
        exit(-1);
    }
    //Wait for servers to finish
    for(i = 0; i < servers; i++) {
        if((terror = pthread_join(service_t[i], NULL))) {
            screen_end();
            printf("Error joing service thread #%d (Code:%d)\n",i,terror);
            exit(-1);
        }
    }
    //Wait for statistics thread
    if((terror = pthread_join(statistics_t, NULL))) {
        screen_end();
        printf("Error joing statistics thread (Code:%d)\n",terror);
        exit(-1);
    }

    wait_for_user();

    /////////////////////////////////////////////////////////////////////////
    //Clean up dynamically allocated memory, mutexes, and semaphores
    screen_end();
    pthread_mutex_destroy(&displock);
    pthread_mutex_destroy(&livelock);
    pthread_mutex_destroy(&deadlock);
    sem_destroy(&customers_left);
    destroy_cqueue(source);
    destroy_cqueue(live);
    destroy_cqueue(dead);
    free(service_t);
    free(servd);
    return 0;
}

void psleep(double interval) {
    struct timespec t;
    t.tv_sec  = (int)floor(interval);
    t.tv_nsec = (interval-t.tv_sec) * 1000000000L;
    nanosleep(&t,NULL);
}

double time_elapsed(timeval f, timeval s) {
    double  sec = (f.tv_sec-s.tv_sec);
    double usec = (f.tv_usec-s.tv_usec)/1000000;
    return (double)(sec+usec);
}

void* genesis(void* targ) {
    genesis_data* gensd = (genesis_data*)targ;
    customer* c = NULL;
    timeval birthday;
    int customers_left;

    srand48(gensd->rseed);

    sem_getvalue(gensd->customers_left, &customers_left);
    while(customers_left > 0) {
        //Get a blank customer from source and initialize it
        gettimeofday(&birthday,NULL);
        c = decqueue(gensd->source);
        c->born = birthday;
        c->job  = rexp(gensd->mu);

        //Enqueue new customer
        pthread_mutex_lock(gensd->livelock);
        encqueue(gensd->live, c);
        pthread_mutex_unlock(gensd->livelock);

        //Decrement customers left and set loop control
        sem_wait(gensd->customers_left);
        sem_getvalue(gensd->customers_left, &customers_left);

        //Sleep until next customer arrives
        psleep(rexp(gensd->lambda));
    }

    return NULL;
}

void* service(void* targ) {
    service_data* servd = (service_data*)targ;
    customer* c = NULL;
    timeval deathday, started, now;
    int customers_left, served = 0;
    double utilized, worked = 0;

    gettimeofday(&started, NULL);
    while(1) {
        //Dequeue live customer
        pthread_mutex_lock(servd->livelock);
        c = decqueue(servd->live);
        pthread_mutex_unlock(servd->livelock);

        //Check to see if there is no more work
        sem_getvalue(servd->customers_left, &customers_left);
        if(customers_left == 0 & c == NULL) {
            break;
        }

        //No customer in line apparently, idle
        if(c == NULL) {
            //Calculate utilization and update display
            gettimeofday(&now,NULL);
            utilized = 100*worked/time_elapsed(now,started);
            pthread_mutex_lock(servd->displock);
            update_server(servd->stid,utilized,served);
            pthread_mutex_unlock(servd->displock);
            psleep(0.01);
            continue;
        }

        //Service customer
        worked += c->job;
        psleep(c->job);
        gettimeofday(&deathday,NULL);
        c->died = deathday;
        served++;

        //Calculate utilization and update display
        utilized = 100*worked/time_elapsed(deathday,started);
        pthread_mutex_lock(servd->displock);
        update_server(servd->stid,utilized,served);
        pthread_mutex_unlock(servd->displock);

        //Enqueue customer in dead queue
        pthread_mutex_lock(servd->deadlock);
        encqueue(servd->dead, c);
        pthread_mutex_unlock(servd->deadlock);
    }

    //Final dipslay update
    gettimeofday(&deathday,NULL);
    utilized = 100*worked/time_elapsed(deathday,started);
    pthread_mutex_lock(servd->displock);
    update_server(servd->stid,utilized,served);
    pthread_mutex_unlock(servd->displock);

    return NULL;
}

void* statistics(void* targ) {
    statistics_data* statd = (statistics_data*)targ;
    timeval started, now;
    customer* c;
    int l, customers_left;
    double t, completed, sigma, average, worked = 0;
    //Variables for sigma of queue length
    int    qlen_ssq = 0; //Sum of the squares of the lengths of queue
    int    qlen_sum = 0; //Sum of the lengths of queue
    int    polled   = 0; //Number of times the queue length was polled
    //Variables for sigma of wait time
    double wait_ssq = 0; //Sum of the squares of the wait time
    double wait_sum = 0; //Sum of the wait time
    int    analyzed = 0; //Number of customers analyzed

    //Initialize statistics output
    pthread_mutex_lock(statd->displock);
    update_wait_stats(0,0);
    update_queue_stats(0,0);
    pthread_mutex_unlock(statd->displock);

    gettimeofday(&started,NULL);
    while(1) {
        //Dequeue dead customer
        pthread_mutex_lock(statd->deadlock);
        c = decqueue(statd->dead);
        pthread_mutex_unlock(statd->deadlock);

        //Get the live customer count
        pthread_mutex_lock(statd->livelock);
        l = statd->live->count;
        pthread_mutex_unlock(statd->livelock);

        //Check to see if there is no more work
        sem_getvalue(statd->customers_left, &customers_left);
        if(customers_left == 0 && l == 0 && c == NULL) {
            break;
        }

        //Update Progress
        gettimeofday(&now,NULL);
        t = time_elapsed(now,started);
        completed = 100*analyzed/(double)statd->customers;
        pthread_mutex_lock(statd->displock);
        update_progress(t,worked/t,completed);
        pthread_mutex_unlock(statd->displock);

        //Update queue length statistics
        polled++;
        qlen_sum += l;
        qlen_ssq += l*l;
        if(polled > 1) {
            average = qlen_sum/polled;
            sigma   = qlen_ssq - (qlen_sum*qlen_sum)/polled;
            sigma   = sigma/(polled-1);
            sigma   = sqrt(sigma);
            pthread_mutex_lock(statd->displock);
            update_queue_stats(average, sigma);
            pthread_mutex_unlock(statd->displock);
        }

        //No customer to analyze
        if(c == NULL) {
            psleep(0.02);
            continue;
        }

        //Analyze all dead customers and destroy them
        while(c != NULL) {
            t = time_elapsed(c->died,c->born);
            analyzed++;
            wait_sum += t;
            wait_ssq += t*t;
            worked   += c->job;
            //Don't need mutex only other thread that
            //uses free and malloc is main and it waits
            //on this thread
            destroy_customer(c);
            //Dequeue dead customer
            pthread_mutex_lock(statd->deadlock);
            c = decqueue(statd->dead);
            pthread_mutex_unlock(statd->deadlock);
        }
        //Update wait statistics
        if(analyzed > 1) {
            average = wait_sum/analyzed;
            sigma   = wait_ssq - (wait_sum*wait_sum)/analyzed;
            sigma   = sigma/(analyzed-1);
            sigma   = sqrt(sigma);
            pthread_mutex_lock(statd->displock);
            update_wait_stats(average, sigma);
            pthread_mutex_unlock(statd->displock);
        }
        psleep(0.02);
    }

    //Update Progress
    gettimeofday(&now,NULL);
    t = time_elapsed(now,started);
    completed = 100*analyzed/(double)statd->customers;
    pthread_mutex_lock(statd->displock);
    update_progress(t,worked/t,completed);
    pthread_mutex_unlock(statd->displock);
    //Final queue length statistics update
    average = qlen_sum/polled;
    sigma   = qlen_ssq - (qlen_sum*qlen_sum)/polled;
    sigma   = sigma/(polled-1);
    sigma   = sqrt(sigma);
    pthread_mutex_lock(statd->displock);
    update_queue_stats(average, sigma);
    pthread_mutex_unlock(statd->displock);
    //Final wait time statistics update
    average = wait_sum/analyzed;
    sigma   = wait_ssq - (wait_sum*wait_sum)/analyzed;
    sigma   = sigma/(analyzed-1);
    sigma   = sqrt(sigma);
    pthread_mutex_lock(statd->displock);
    update_wait_stats(average, sigma);
    pthread_mutex_unlock(statd->displock);

    return NULL;
}

