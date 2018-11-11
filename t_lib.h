#ifndef tlib_h
#define tlib_h

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

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

typedef struct priorityQueue
{
	struct QNode *front;
}priorityQueue;

void t_yield();
void t_init();
void t_shutdown();
void t_terminate();
void enQueue(Queue *q, tcb *tcb);
int t_create(void (*fct)(int), int id, int pri);
Queue* createQueue();
priorityQueue *createPriQueue();
QNode* newNode(tcb *key);
QNode* deQueue(Queue *q);
void sig_hand(int signo);
void init_alarm();
void push(priorityQueue *q, QNode *node);
QNode *pop(priorityQueue *q);
void printList(priorityQueue *queueHead);

#endif //tlib_h