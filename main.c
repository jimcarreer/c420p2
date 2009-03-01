#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "customer.h"

//Simulation defaults
double lambda = 3.0;   //Arrival time distribution parameter
double mu     = 4.0;   //Service time distribution parameter
int    cnum   = 1000;  //Number of customers for simulation
int    snum   = 1;     //Number of servers for simulation
double seed   = 0;     //Random number generator seed
cqmode mode   = FIFO;  //Operational mode

//Working functions
inline double rexp(double l) {return -log(1.0-drand48())/l;}


int main()
{

}

