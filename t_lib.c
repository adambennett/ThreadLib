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
	
	sem_init(&(thread)->locking, 0);
	sem_init(&(thread)->counting, 0);
	
	thread->mBit = -1;
	thread->q = calloc(100, sizeof(char*));
	thread->thread_id = id;
	thread->thread_priority = pri;
	thread->thread_context = uc;
	enQueue(ready, thread);
	enQueue(threads, thread);
}


void t_shutdown()
{
	tcb *temp;
	QNode *current = ready->front;
	//QNode *current2 = threads->front;
	
	//Free readyQueue data - QNodes, TCBs, and context references
	while(current != NULL)
	{
		temp = current->key;
		if(temp->thread_id > 0) { free(temp->thread_context->uc_stack.ss_sp);}
		free(temp->thread_context);
		free(temp);
		free(current);
		current = current->next;
	}
	
	/*
	//Free global thread list data - QNodes, TCBs, and context references
	while(current2 != NULL)
	{
		temp = current2->key;
		if(temp->thread_id > 0) { free(temp->thread_context->uc_stack.ss_sp);}
		free(temp->thread_context);
		free(temp);
		free(current2);
		current2 = current2->next;
	}
	*/
	
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
	//free(temp->thread_context->uc_stack.ss_sp);
	//free(temp->thread_context);
	//free(temp);
	
	//Dequeue from ready queue
	current = deQueue(ready);

	//Run new thread and free temp reference node
	running = current->key;
	
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
	   temp->prev = NULL;
	   temp->next = NULL;
	   return; 
	} 

	// Add the new node at the end of queue and change rear 
	q->rear->next = temp; 
	temp->prev = q->rear;
	q->rear = temp; 
	temp->next = NULL;
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
	   temp->prev = NULL;
	   temp->next = NULL;
	   return; 
	} 

	// Add the new node at the end of queue and change rear 
	q->rear->next = temp; 
	temp->prev = q->rear;
	q->rear = temp; 
	temp->next = NULL;
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

/* Function to delete a node in a Doubly Linked List. 
   head_ref --> pointer to head node pointer. 
   del  -->  pointer to node to be deleted. */
void deleteNode(MNode **head_ref, MNode *del) 
{ 
  /* base case */
  if(*head_ref == NULL || del == NULL) 
    return; 
  
  /* If node to be deleted is head node */
  if(*head_ref == del) 
    *head_ref = del->next; 
  
  /* Change next only if node to be deleted is NOT the last node */
  if(del->next != NULL) 
    del->next->prev = del->prev; 
  
  /* Change prev only if node to be deleted is NOT the first node */
  if(del->prev != NULL) 
    del->prev->next = del->next;      
  
  /* Finally, free the memory occupied by del*/
  free(del); 
  return; 
}

/* Function to delete a node in a Doubly Linked List. 
   head_ref --> pointer to head node pointer. 
   del  -->  pointer to node to be deleted. */
void deleteQNode(QNode **head_ref, QNode *del) 
{ 
  /* base case */
  if(*head_ref == NULL || del == NULL) 
    return; 
  
  /* If node to be deleted is head node */
  if(*head_ref == del) 
    *head_ref = del->next; 
  
  /* Change next only if node to be deleted is NOT the last node */
  if(del->next != NULL) 
    del->next->prev = del->prev; 
  
  /* Change prev only if node to be deleted is NOT the first node */
  if(del->prev != NULL) 
    del->prev->next = del->next;      
  
  /* Finally, free the memory occupied by del*/
  free(del); 
  return; 
}

MNode *search(MNode* head, int x) 
{ 
    MNode* current = head;  // Initialize current 
    while (current != NULL) 
    { 
        if (current->key->sender == x) 
            return current; 
        current = current->next; 
    } 
    return NULL; 
}   

QNode *search2(QNode* head, QNode* del) 
{ 
    QNode* current = head;  // Initialize current 
    while (current != NULL) 
    { 
        if (current == del) 
            return current; 
        current = current->next; 
    } 
    return NULL; 
}  

QNode *search3(QNode* head, int tid) 
{ 
	//printList(threads->front);
    QNode* current = head;  // Initialize current 
    while (current != NULL) 
    { 
        if (current->key->thread_id == tid) 
            return current; 
        current = current->next; 
    } 
    return NULL; 
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
	sem_init(&(*mb)->s, 0);
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

mcb *newMsg(char *msg, int len, int sender, int rec)
{
	mcb *temp = malloc(sizeof(mcb));
	temp->message = malloc(sizeof(msg) * len);
	strcpy(temp->message, msg);
	temp->len = len;
	temp->next = NULL;
	temp->sender = sender;
	temp->receiver = rec;
	return temp;
}

void send(int tid, char *msg, int len)
{
	// look through global thread list to find thread with ID = tid
	QNode *thread = search3(threads->front, tid);
	// lock thread message queue
	// 
	// put msg into thread message queue
	mcb* temp = newMsg(msg, len, running->thread_id, tid);
	enQueueMsg(thread->key->q, temp);
	// unlock thread message queue
	//
	// check tid thread int bit to see if it is waiting for this message
	if (thread->key->mBit == tid)
	{
		// sem_post() on thread sem_t to wake up thread if it is
		sem_signal(thread->key->counting);
		// reset thread waiting for int bit if so
		thread->key->mBit == -1;
	}
	return;
}

void receive(int *tid, char *msg, int *len)
{
	// look through global thread list to find thread with ID = tid
	QNode *thread = search3(threads->front, running->thread_id);
	if (*tid > 0)
	{
		// lock thread message queue
		// 
		// look through thread msg queue for message matching tid
		MNode *msgNode = search(thread->key->q->front, *tid);
		//if (msgNode != NULL) { printf("%s\n", msgNode->key->message); }
		// if there is one
		if (msgNode != NULL)
		{
			// Set msg = found message
			sprintf(msg, "%s", msgNode->key->message);
			*len = msgNode->key->len;
			// unlock queue
			//
			// reset thread int bit to -1
			thread->key->mBit = -1;
			return;
		}
		// if no matching message
		else
		{
			// set thread int bit to tid
			thread->key->mBit = *tid;
			// unlock queue
			//
			// sem_wait() on thread sem_t
			sem_wait(thread->key->counting);
			// recursive receive() call
			receive(tid, msg, len);
			//printf("First else in receive() triggered\n");
		}
	}
	// if tid = 0
	else if (*tid == 0)
	{
		// lock thread message queue
		//
		// if the message queue is not empty
		if (thread->key->q->front != NULL)
		{
			// Set msg = first message in queue
			MNode *msgNode = deQueueMsg(thread->key->q);
			sprintf(msg, "%s", msgNode->key->message);
			*len = msgNode->key->len;
			// unlock queue
			//
			// reset thread int bit to -1
			thread->key->mBit = -1;
			return;
		}
		// if message queue IS empty
		else
		{
			// set thread int bit to 0
			thread->key->mBit = 0;
			// unlock queue
			//
			// sem_wait on thread sem_t
			sem_wait(thread->key->counting);
			// recursive receive() call
			receive(tid, msg, len);
			//printf("Second else in receive() triggered\n");
		}
	}
}

// This function prints contents of linked list starting from the given node 
void printList(QNode* node) 
{ 
    QNode* last; 
    printf("\nTraversal in forward direction \n"); 
    while (node != NULL) { 
        printf(" %d ", node->key->thread_id); 
        last = node; 
        node = node->next; 
    } 
  
	printf("\n");
	/*
    printf("\nTraversal in reverse direction \n"); 
    while (last != NULL) { 
        printf(" %d ", last->key->thread_id); 
        last = last->prev; 
    } 
	*/
} 

// This function prints contents of linked list starting from the given node 
void printMessages(MNode* node) 
{ 
    MNode* last; 
    printf("\nTraversal in forward direction \n"); 
    while (node != NULL) { 
        printf(" %s\n", node->key->message); 
        last = node; 
        node = node->next; 
    } 
  
	printf("\n");
	/*
    printf("\nTraversal in reverse direction \n"); 
    while (last != NULL) { 
        printf(" %d ", last->key->thread_id); 
        last = last->prev; 
    } 
	*/
} 



