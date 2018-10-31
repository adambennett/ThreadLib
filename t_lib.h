#ifndef tlib_h
#define tlib_h

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/mman.h>

// Node structure for use in Queues
typedef struct QNode 
{ 
	struct tcb *key; 
	struct QNode *next; 
}QNode; 

// Thread control block structure
typedef struct tcb 
{
	int         thread_id;
	int         thread_priority;
	ucontext_t  *thread_context;
}tcb;

//The queue, 
//Front = first element of Linked List
//Read = last element of Linked List
typedef struct Queue 
{ 
	struct QNode *front, *rear; 
}Queue; 

void t_yield();
void t_init();
void t_shutdown();
void t_terminate();
struct QNode* newNode(tcb *key);
struct Queue *createQueue();
void enQueue(Queue *q, tcb *tcb);
void enQueueExisting(Queue *q, QNode *temp);
struct QNode *deQueue(Queue *q);
int t_create(void (*fct)(int), int id, int pri);

#endif //tlib_h