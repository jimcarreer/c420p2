#include "customer.h"

//The GOOD functions
static void _encqueue_fifo(cqueue* q, customer* c);
static void _encqueue_sjf(cqueue* q,  customer* c);

customer* decqueue(cqueue* q) {
    customer * c = NULL;
    if(q == NULL)
        return NULL;
    if(q->head == NULL)
        return NULL;
    c = q->head;

    q->head = c->next;
    //This will be true except when there is 1 node left
    if(q->head != NULL)
        q->head->prev = NULL;
    q->count--;

    //List is now empty reset all
    if(q->head == NULL) {
        q->tail  = NULL;
        q->count = 0;
    }

    //Don't want any loose ends
    c->next = NULL;
    c->prev = NULL;
    return c;
}

void encqueue(cqueue* q, customer* c) {
    if(q == NULL)
        return;
    //Enqueue based on what mode passed
    switch(q->mode) {
        case FIFO: return _encqueue_fifo(q,c);
        case SJF : return _encqueue_sjf(q,c);
        default  : return _encqueue_fifo(q,c);
    }
}

cqueue* new_cqueue(cqmode m) {
    cqueue* q = (cqueue*)malloc(sizeof(cqueue));
    q->mode   = m;
    q->head   = NULL;
    q->tail   = NULL;
    q->count  = 0;
    return q;
}

customer* new_customer(double j, timeval b) {
    customer* c = (customer*)malloc(sizeof(customer));
    c->job      = j;
    c->born     = b;
    c->next     = NULL;
    c->prev     = NULL;
    return c;
}

customer* new_customer(void) {
    customer* c = (customer*)malloc(sizeof(customer));
    c->next     = NULL;
    c->prev     = NULL;
    return c;
}

void destroy_cqueue(cqueue* q) {
    customer* i = decqueue(q);
    while(i != NULL) {
        destroy_customer(i);
        i = decqueue(q);
    }
    free(q);
    return;
}

void destroy_customer(customer* c) {
    free(c);
    return;
}

void print_cqueue_trace(cqueue q) {
    customer* i = q.head;
    if(q.head == NULL) {
        printf("Queue is empty\n");
        return;
    }
    printf("Count : %d\n",q.count);
    switch(q.mode) {
        case SJF  : printf("Mode  : SJF\n");  break;
        case FIFO : printf("Mode  : FIFO\n"); break;
    }
    printf("Address\t\t| Next\t\t| Previous\t| Job\n");
    while(i != NULL) {
        printf("%p\t|",i);
        if(i->next == NULL)
            printf(" %p\t\t|",i->next);
        else
            printf(" %p\t|",i->next);
        if(i->prev == NULL)
            printf(" %p\t\t|",i->prev);
        else
            printf(" %p\t|",i->prev);
        printf(" %lf\n",i->job);
        i = i->next;
    }
    return;
}

void _encqueue_fifo(cqueue* q, customer* c) {
    if(q == NULL || c == NULL)
        return;
    //Queue is empty
    if(q->head == NULL) {
        q->head =  c;
        q->tail =  c;
        q->count = 1;
    } else {
        q->tail->next = c;
        c->prev = (customer*)q->tail;
        q->tail = c;
        q->count++;
    }
    return;
}

void _encqueue_sjf(cqueue* q, customer* c) {
    if(q == NULL || c == NULL)
        return;
    //Queue is empty
    if(q->head == NULL) {
        return _encqueue_fifo(q,c);
    //Node goes at head
    } else if(c->job <= q->head->job) {
        c->next = q->head;
        q->head->prev = c;
        q->head = c;
        q->count++;
        return;
    //Node goes at the tail
    } else if(c->job >= q->tail->job) {
        return _encqueue_fifo(q,c);
    //Node goes somewhere in between
    } else {
        customer* i = q->head;
        while(i != NULL) {
            if(i->job >= c->job) {
                //We add the new node after if jobs are equal
                //i->next should never be null because of the
                //third block of this if-else statement
                if(i->job == c->job)
                    i = i->next;
                c->next = i;
                i->prev->next = c;
                c->prev = i->prev;
                i->prev = c;
                q->count++;
                break;
            }
            i = i->next;
        }
    }
    return;
}
