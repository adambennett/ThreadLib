#include "t_lib.h"

tcb *running;
Queue *ready;
Queue *threads;

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
	tmp->alive = 1;
	
	//Init ready queue
	ready = createQueue();
	threads = createQueue();
	
	//Add main thread to global thread list
	enQueue(threads, tmp);
	
	//Set running to point to temp
	running = tmp;
}

void t_create(void (*fct)(int), int id, int pri)
{
	// Generic thread init stuff
	size_t sz = 0x10000;
	ucontext_t *uc;
	uc = (ucontext_t *) malloc(sizeof(ucontext_t));
	getcontext(uc);
	uc->uc_stack.ss_sp = malloc(sz);
	uc->uc_stack.ss_size = sz;
	uc->uc_stack.ss_flags = 0;
	uc->uc_link = running; 
	
	// Set thread function
	makecontext(uc, (void (*)(void)) fct, 1, id);

	// Create thread control block for new thread
	tcb *thread = (tcb *) malloc(sizeof(tcb));
	
	// Init thread semaphores for locking and blocking
	sem_init(&(thread)->locking, 1);
	sem_init(&(thread)->counting, 0);
	
	// Set int bit that keeps track of what thread's message this thread is waiting to receive
	thread->mBit = -1;	// -1 is flag for not waiting
	
	// Set thread to alive
	thread->alive = 1;
	
	// Allocate space for messages to be held within each thread
	thread->q = createMsgQueue();
	
	// Thread ID, priority, context ref
	thread->thread_id = id;
	thread->thread_priority = pri;
	thread->thread_context = uc;
	
	// Put new thread into both the ready queue (so it will be executed) and the threads queue (so it can receive messages)
	enQueue(ready, thread);
	enQueue(threads, thread);
}

void t_shutdown()
{
	tcb *temp;
	QNode *current = ready->front;
	QNode *current2 = threads->front;
	QNode *ref;
	MNode *ref2;
	
	//Free readyQueue data - QNodes, TCBs, context refs, semaphores, and message queues
	while(current != NULL)
	{
		temp = current->key;
		if(temp->thread_id > 0) { free(temp->thread_context->uc_stack.ss_sp);}
		free(temp->thread_context);
		sem_destroy(&(temp)->locking);
		sem_destroy(&(temp)->counting);
		MNode *current3 = temp->q->front;
		while(current3 != NULL)
		{
			mcb *temp2 = current3->key;
			free(temp2->message);
			free(temp2);
			ref2 = current3->next;
			free(current3);
			current3 = ref2;
		}
		msgQueuePlumber(temp->q);
		free(temp);
		ref = current->next;
		free(current);
		current = ref;
	}
	
	//Free global thread list data - QNodes, TCBs, and context references
	while(current2 != NULL)
	{
		temp = current2->key;
		if(temp->thread_id > 0) { free(temp->thread_context->uc_stack.ss_sp);}
		current2 = current2->next;
	}
	
	//Free running thread data - QNode, TCB, and context reference
	temp = running;
	if(temp->thread_id > 0) {free(temp->thread_context->uc_stack.ss_sp);}
	free(temp->thread_context);
	free(temp);
	
	//Free queue struct
	free(ready);
	
	//Free global thread list
	free(threads);
	
}

void t_terminate()
{
	tcb *temp;
	QNode *current;
	temp = running;
	QNode *staticRunRef = newNode(temp);
	MNode *ref2;
	
	//Free running thread
	free(temp->thread_context->uc_stack.ss_sp);
	free(temp->thread_context);
	sem_destroy(&(temp)->locking);
	sem_destroy(&(temp)->counting);
	MNode *current3 = temp->q->front;
	while(current3 != NULL)
	{
		mcb *temp2 = current3->key;
		free(temp2->message);
		free(temp2);
		ref2 = current3->next;
		free(current3);
		current3 = ref2;
	}
	free(current3);
	msgQueuePlumber(temp->q);
	free(temp);
	
	//Dequeue from ready queue
	current = deQueue(ready);

	//Run new thread and free temp reference node
	running = current->key;
	
	//Reset a reference to the terminated thread inside the global thread list
	staticRunRef->key->alive = 0;
	enQueue(threads, staticRunRef->key);
	
	//Context switch
	setcontext(running->thread_context);
}

// Initialize semaphore
int sem_init(sem_t **s, int sem_count)
{
	*s = malloc(sizeof(sem_t));
	(*s)->count = sem_count;
	(*s)->q = createQueue();
}

// Decrement passed semaphore
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

// Incremenet passed semaphore : aka sem_post()
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

// Terminates and frees semaphore
void sem_destroy(sem_t **s)
{
	tcb *temp;
	QNode *current = (*s)->q->front;
	QNode *ref;
	MNode *ref2;
	
	//Free readyQueue data - QNodes, TCBs, and context references
	while(current != NULL)
	{
		temp = current->key;
		if(temp->thread_id > 0) {free(temp->thread_context->uc_stack.ss_sp);}
		free(temp->thread_context);
		MNode *current3 = temp->q->front;
		while(current3 != NULL)
		{
			mcb *temp2 = current3->key;
			free(temp2->message);
			free(temp2);
			ref2 = current3->next;
			free(current3);
			current3 = ref2;
		}
		msgQueuePlumber(temp->q);
		free(temp);
		ref = current->next;
		free(current);
		current = ref;
	}
	
	//Free the semaphore queue struct and semaphore
	free((*s)->q);
	free(*s);
	return;
}

// Create mbox object and allocate necessary space
int mbox_create(mbox **mb)
{
	*mb = malloc(sizeof(mbox));	
	(*mb)->q = createMsgQueue();
	sem_init(&(*mb)->s, 1);
}

// Terminate and free mbox object
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

// Put message into mbox queue
void mbox_deposit(mbox *mb, char *msg, int len)
{
	// Create temp message control block
	mcb *temp = malloc(sizeof(mcb));
	
	// Allocate mcb message field
	temp->message = malloc(sizeof(msg) * len);
	
	// Copy desired message into mcb message field
	strcpy(temp->message, msg);
	
	// Copy message length into mcb len field
	temp->len = len;
	temp->next = NULL;
	
	// Lock mbox queue
	sem_wait(mb->s);
	
	// Enqueue message into queue
	enQueueMsg(mb->q, temp);
	
	// Unlock mbox queue
	sem_signal(mb->s);
}

void mbox_withdraw(mbox *mb, char *msg, int *len)
{
	// Lock mbox queue
	sem_wait(mb->s);
	
	// Dequeue message from mbox
	MNode *temp = deQueueMsg(mb->q);
	
	// Unlock queue
	sem_signal(mb->s);
	
	// Point msg and len to the found message and the length of it
	sprintf(msg, "%s", temp->key->message);
	*len = temp->key->len;
}

// Sends msg from running thread to thread with thread_id = tid
void send(int tid, char *msg, int len)
{
	// Find the thread we want to send to within global thread list
	QNode *thread = search2(threads->front, tid);
	
	// Lock thread message queue
	sem_wait(thread->key->locking);
	
	// Create message control block for new message
	mcb* temp = newMsg(msg, len, running->thread_id, tid);
	
	// Enqueue message into thread queue
	enQueueMsg(thread->key->q, temp);
	
	// Unlock thread message queue
	sem_signal(thread->key->locking);
	
	// If thread we just deposited into is waiting on this message
	if (thread->key->mBit == tid || thread->key->mBit == 0)
	{
		// Wake thread
		sem_signal(thread->key->counting);
		
		// Reset wait bit
		thread->key->mBit == -1;
	}
	return;
}

// Receives a msg in running thread from thread with thread_id = tid, or if tid = 0 blocks until a message is received
void receive(int *tid, char *msg, int *len)
{
	// Find the thread we want to send to within global thread list
	QNode *thread = search2(threads->front, running->thread_id);
	
	// If looking for specific thread's message
	if (*tid > 0)
	{
		// Lock message queue
		sem_wait(thread->key->locking);
		
		// Look through queue for matching message
		MNode *msgNode = search(thread->key->q->front, *tid);
		
		// If message match found
		if (msgNode != NULL)
		{
			// Copy and remove message from queue
			sprintf(msg, "%s", msgNode->key->message);
			*len = msgNode->key->len;
			deleteNode(&(thread)->key->q->front, msgNode, thread->key->q); 
			
			// Unlock message queue and reset wait bit
			sem_signal(thread->key->locking);
			thread->key->mBit = -1;
			return;
		}
		else
		{
			// Set wait bit and unlock message queue
			thread->key->mBit = *tid;
			sem_signal(thread->key->locking);
			
			// Block until message is deposited matching wait bit sender
			sem_wait(thread->key->counting);
			
			// When message is received, just call this again to properly receive it
			receive(tid, msg, len);
		}
	}

	// If looking for any message
	else if (*tid == 0)
	{
		// Lock message queue
		sem_wait(thread->key->locking);
		
		// If message queue is not empty
		if (thread->key->q->front != NULL)
		{
			// Remove message from queue and copy data to proper locations
			MNode *msgNode = deQueueMsg(thread->key->q);
			sprintf(msg, "%s", msgNode->key->message);
			*len = msgNode->key->len;
			*tid = msgNode->key->receiver;
			
			// Unlock message queue and reset wait bit
			sem_signal(thread->key->locking);
			thread->key->mBit = -1;
			return;
		}
		else
		{
			// Set wait bit and unlock message queue
			thread->key->mBit = 0;
			sem_signal(thread->key->locking);
			
			// Block until message is received
			sem_wait(thread->key->counting);
			
			// When message is received, just call this again to properly receive it
			receive(tid, msg, len);
		}
	}
}