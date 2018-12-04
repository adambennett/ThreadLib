#ifndef tstruct_h
#define tstruct_h

// Node structure for use Thread Queues
typedef struct QNode 
{ 
	struct tcb *key; 
	struct QNode *next; 
	struct QNode *prev;
}QNode; 

// Node structure for use in Message Queues
typedef struct MNode 
{ 
	struct mcb *key; 
	struct MNode *next; 
	struct MNode *prev;
}MNode; 

// Thread control block structure
typedef struct tcb 
{
	int         		thread_id;
	int         		thread_priority;
	int					mBit;				// int bit for send/receive (keeps track of what the thread is waiting on)
	int					alive;				// Set to 0 if this thread is terminated but still exists within the global thread list
	ucontext_t  		*thread_context;
	struct tcb 			*next;					
	struct sem_t 		*locking;			// for send/receive
	struct sem_t		*counting;			// for send/receive
	struct msgQueue 	*q;					// message queue
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

#endif //tstruct.h