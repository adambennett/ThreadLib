#include "t_lib.h"

tcb *running;
Queue *ready;

void t_yield()
{
	tcb *temp;
	temp = running;
	QNode *new = deQueue(ready);

	if(new != NULL)
	{
		//Reset running to dequeued thread
		running = new->key;
		
		free(new);
		
		//Enqueue old running thread to ready queue
		enQueue(ready, temp);
		
		//Switch contexts
		swapcontext(temp->thread_context, running->thread_context);
	}
}

void t_init()
{
	// temp will be the main thread's context ref
	ucontext_t *temp;
	temp = (ucontext_t *) malloc(sizeof(ucontext_t));
	getcontext(temp);
	
	//Setup main thread
	tcb *tmp = (tcb *) malloc(sizeof(tcb));
	tmp->thread_id = 0;
    tmp->thread_priority = 0;
	tmp->thread_context = temp;
	
	//Init ready queue
	ready = createQueue();
	
	//Set running to point to temp
	running = tmp;
}


void t_create(void (*fct)(int), int id, int pri)
{
	size_t sz = 0x10000;

	ucontext_t *uc;
	uc = (ucontext_t *) malloc(sizeof(ucontext_t));

	getcontext(uc);
	/***
	uc->uc_stack.ss_sp = mmap(0, sz,
	   PROT_READ | PROT_WRITE | PROT_EXEC,
	   MAP_PRIVATE | MAP_ANON, -1, 0);
	***/
	uc->uc_stack.ss_sp = malloc(sz);  /* new statement */
	uc->uc_stack.ss_size = sz;
	uc->uc_stack.ss_flags = 0;
	uc->uc_link = running; 
	makecontext(uc, (void (*)(void)) fct, 1, id);

	tcb *thread = (tcb *) malloc(sizeof(tcb));
	thread->thread_id = id;
	thread->thread_priority = pri;
	thread->thread_context = uc;
	enQueue(ready, thread);
}


void t_shutdown()
{
	tcb *temp;
	QNode *current = ready->front;
	
	//Free readyQueue data - QNodes, TCBs, and context references
	while(current != NULL)
	{
		temp = current->key;
		if(temp->thread_id > 0) {free(temp->thread_context->uc_stack.ss_sp);}
		free(temp->thread_context);
		free(temp);
		free(current);
		current = current->next;
	}
	
	//Free running thread data - QNode, TCB, and context reference
	temp = running;
	if(temp->thread_id > 0) {free(temp->thread_context->uc_stack.ss_sp);}
	free(temp->thread_context);
	free(temp);
	
	//Free queue struct
	free(ready);
	
}

void t_terminate()
{
	tcb *temp;
	QNode *current;
	temp = running;
	
	//Free running thread
	free(temp->thread_context->uc_stack.ss_sp);
	free(temp->thread_context);
	free(temp);
	
	//Dequeue from ready queue
	current = deQueue(ready);
	
	//Run new thread and free temp reference node
	running = current->key;
	free(current);
	
	setcontext(running->thread_context);
}

// A utility function to create a new thread queue node
QNode *newNode(tcb *tcb)
{
    QNode *temp = (QNode*)malloc(sizeof(QNode)); 
    temp->key = tcb; 
    temp->next = NULL; 
    return temp;  
}

// A utility function to create a new message queue node
MNode *newMsgNode(mcb *mcb)
{
    MNode *temp = (MNode*)malloc(sizeof(MNode)); 
    temp->key = mcb; 
    temp->next = NULL; 
    return temp;  
}

// A utility function to create an empty thread queue 
Queue *createQueue() 
{
	Queue *q = (Queue*)malloc(sizeof(Queue)); 
    q->front = q->rear = NULL; 
    return q; 
}

// A utility function to create an empty message queue 
msgQueue *createMsgQueue() 
{
	msgQueue *q = (msgQueue*)malloc(sizeof(msgQueue)); 
    q->front = q->rear = NULL; 
    return q; 
}

// The function to add a thread control block into a queue
void enQueue(Queue *q, tcb *tcb) 
{ 
	// Create a new LL node with the properties of the thread control block
	QNode *temp = newNode(tcb);
	
	// If queue is empty, then new node is both front and rear
	if (q->rear == NULL) 
	{ 
	   q->front = q->rear = temp; 
	   return; 
	} 

	// Add the new node at the end of queue and change rear 
	q->rear->next = temp; 
	q->rear = temp; 
}

// The function to add a message control block into a queue
void enQueueMsg(msgQueue *q, mcb *mcb) 
{ 
	// Create a new LL node with the properties of the thread control block
	MNode *temp = newMsgNode(mcb);
	
	// If queue is empty, then new node is both front and rear
	if (q->rear == NULL) 
	{ 
	   q->front = q->rear = temp; 
	   return; 
	} 

	// Add the new node at the end of queue and change rear 
	q->rear->next = temp; 
	q->rear = temp; 
}

// Function to remove a thread from given queue q
QNode *deQueue(Queue *q) 
{ 
    //If the queue is empty return NULL
    if (q->front == NULL) 
	{
       return NULL; 
	}
  
    //Store previous front and move front one node ahead 
	QNode *temp = q->front; 
    q->front = q->front->next; 
  
    //If front becomes NULL, then change rear to NULL as well
    if (q->front == NULL)
	{		
       q->rear = NULL; 
	}
	
	//Return the node we removed
    return temp; 
}

// Function to remove a message from given queue q
MNode *deQueueMsg(msgQueue *q) 
{ 
    //If the queue is empty return NULL
    if (q->front == NULL) 
	{
       return NULL; 
	}
  
    //Store previous front and move front one node ahead 
	MNode *temp = q->front; 
    q->front = q->front->next; 
  
    //If front becomes NULL, then change rear to NULL as well
    if (q->front == NULL)
	{		
       q->rear = NULL; 
	}
	
	//Return the node we removed
    return temp; 
}

int sem_init(sem_t **s, int sem_count)
{
	*s = malloc(sizeof(sem_t));
	(*s)->count = sem_count;
	(*s)->q = createQueue();
}

void sem_wait(sem_t *s)
{
	//Disable interrupts
	sighold();
	
	// Running thread reference
	tcb *temp = running;
	
	// Decrement semaphore counter
	s->count--;
	
	//If semaphore counter is negative, need to enqueue threads to wait
	if(s->count < 0)
	{
		// Enqueue running thread to semaphore queue
		enQueue(s->q, temp);
		
		// Dequeue from ready queue and switch running thread
		QNode *tempNode = deQueue(ready);
		running = tempNode->key;
		free(tempNode);
		
		// Now swap to new thread from ready queue and reallow interrupts
		swapcontext(temp->thread_context, running->thread_context);
		sigrelse();
		return;
	}
	
	//Otherwise just return
	else if(s->count >= 0) { sigrelse(); return; }
}

void sem_signal(sem_t *s)
{
	//Disable interrupts
	sighold();
	
	//Increment semaphore counter
	s->count++;
	
	//Check if any threads are waiting on semaphore
	//If there are threads waiting on the semaphore, enqueue one to ready queue
	if(s->count <= 0)
	{
		// Dequeue from semaphore queue
		QNode *semaThread = deQueue(s->q);
		
		//Error check
		if(semaThread == NULL) { printf("\nError: sem_signal adding NULL thread\n"); }
		
		//Enqueue waiting thread to ready queue 
		enQueue(ready, semaThread->key);

		// Free temp reference and reallow interrupts
		free(semaThread);
		sigrelse();
		return;
	}
	
	//Otherwise no thread is waiting
	else { sigrelse(); return; }
}

void sem_destroy(sem_t **s)
{
	tcb *temp;
	QNode *current = (*s)->q->front;
	
	//Free readyQueue data - QNodes, TCBs, and context references
	while(current != NULL)
	{
		temp = current->key;
		if(temp->thread_id > 0) {free(temp->thread_context->uc_stack.ss_sp);}
		free(temp->thread_context);
		free(temp);
		free(current);
		current = current->next;
	}
	
	//Free the semaphore queue struct and semaphore
	free((*s)->q);
	free(*s);
	return;
}

int mbox_create(mbox **mb)
{
	*mb = malloc(sizeof(mbox));	
	(*mb)->q = createMsgQueue();
	//sem_t **s = (*mb)->s;
	//sem_init(s, 0);
}

void mbox_destroy(mbox **mb)
{
	mcb *temp;
	MNode *current = (*mb)->q->front;
	
	//Free messageQueue data - MNodes, MCBs, and context references
	while(current != NULL)
	{
		temp = current->key;
		free(temp->message);
		free(temp);
		free(current);
		current = current->next;
	}
	
	//Free msgQueue struct
	free((*mb)->q);
	
	//Free mailbox
	free(*mb);
}

void mbox_deposit(mbox *mb, char *msg, int len)
{
	mcb *temp = malloc(sizeof(mcb));
	temp->message = malloc(sizeof(msg) * len);
	strcpy(temp->message, msg);
	temp->len = len;
	temp->next = NULL;
	enQueueMsg(mb->q, temp);
}

void mbox_withdraw(mbox *mb, char *msg, int *len)
{
	
	MNode *temp = deQueueMsg(mb->q);
	sprintf(msg, "%s", temp->key->message);
	*len = temp->key->len;
}

void send(int tid, char *msg, int len)
{
	
}

void receive(int *tid, char *msg, int *len)
{
	
}
