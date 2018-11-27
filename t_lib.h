#ifndef tlib_h
#define tlib_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <sys/mman.h>

// Node structure for use Thread Queues
typedef struct QNode 
{ 
	struct tcb *key; 
	struct QNode *next; 
}QNode; 

// Node structure for use in Message Queues
typedef struct MNode 
{ 
	struct mcb *key; 
	struct MNode *next; 
}MNode; 

// Thread control block structure
typedef struct tcb 
{
	int         thread_id;
	int         thread_priority;
	ucontext_t  *thread_context;
	struct tcb *next;
}tcb;

// Message control block structure
typedef struct mcb 
{
	char *message;     // copy of the message 
	int  len;          // length of the message 
	int  sender;       // TID of sender thread 
	int  receiver;     // TID of receiver thread 
	struct mcb *next; // pointer to next node 
}mcb;

//Thread queue
//Front = first element of Linked List
//Rear = last element of Linked List
typedef struct Queue 
{ 
	struct QNode *front, *rear; 
}Queue; 

//Message queue
//Front = first element of Linked List
//Rear = last element of Linked List
typedef struct msgQueue 
{ 
	struct MNode *front, *rear; 
}msgQueue;

typedef struct sem_t 
{
  int count;
  Queue *q;
}sem_t;


typedef struct mbox{
	struct msgQueue *q;		// message queue
	sem_t           *s;		// mbox semaphore
}mbox;

void t_yield();
void t_init();
void t_shutdown();
void t_terminate();
void enQueue(Queue *q, tcb *tcb);
void enQueueMsg(msgQueue *q, mcb *mcb);
void t_create(void (*fct)(int), int id, int pri);
void sem_wait(sem_t *s);
void sem_signal(sem_t *s);
void sem_destroy(sem_t **s);
int sem_init(sem_t **s, int sem_count);
QNode* newNode(tcb *key);
MNode *newMsgNode(mcb *mcb);
QNode *deQueue(Queue *q);
MNode *deQueueMsg(msgQueue *q);
Queue *createQueue();
msgQueue *createMsgQueue();
int mbox_create(mbox **mb);
void mbox_destroy(mbox **mb);
void mbox_deposit(mbox *mb, char *msg, int len);
void mbox_withdraw(mbox *mb, char *msg, int *len);
void send(int tid, char *msg, int len);
void receive(int *tid, char *msg, int *len);

#endif //tlib_h