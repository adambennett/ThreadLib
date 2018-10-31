#include "t_lib.h"

ucontext_t *running;
Queue *readyQueue;
Queue *runningQueue;

//PHASE 1
void t_yield()
{
	QNode *prevReady = deQueue(readyQueue);
	if (prevReady != NULL)
	{
		QNode *prevRunning = deQueue(runningQueue);
		enQueue(runningQueue, prevReady->key);
		enQueue(readyQueue, prevRunning->key);
		running = prevReady->key->thread_context;
		swapcontext(prevRunning->key->thread_context, running);
		free(prevReady);
		free(prevRunning);
	}
}

//PHASE 1
void t_init()
{
	// tmp will be the main thread's context ref
	ucontext_t *tmp;
	tmp = (ucontext_t *) malloc(sizeof(ucontext_t));

	getcontext(tmp);    
	running = tmp;
  
	// Create main thread and init ID, priority and context
	tcb *main = (tcb *) malloc(sizeof(tcb));
	main->thread_id = 0;
	main->thread_priority = 0;
	main->thread_context = tmp;

	// Initialize running and ready queues
	readyQueue = createQueue(); 
	runningQueue = createQueue(); 
	
	// Insert main thread into running queue
	enQueue(runningQueue, main);
}

//PHASE 1
int t_create(void (*fct)(int), int id, int pri)
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
	enQueue(readyQueue, thread);
}

//PHASE 1
void t_shutdown()
{
	QNode *temp;
	QNode *current = runningQueue->front;
	
	// Free readyQueue data - QNodes, TCBs, and context references
	while (readyQueue->front != NULL)
	{
		temp = current;
		if (current->next != NULL) { current = current->next; }
		else { current = NULL; }
		
		if (temp->key->thread_id > 0) { free(temp->key->thread_context->uc_stack.ss_sp); }
		
		free(temp->key->thread_context);
		free(temp->key);
		free(temp);
	}
	
	// Free runningQueue data - QNode, TCB, and context reference
	temp = runningQueue->front;
	free(temp->key->thread_context);
	free(temp->key);
	free(temp);
	
	// Free queue structs
	free(readyQueue);
	free(runningQueue);
}

//PHASE 1
void t_terminate()
{
	// Dequeue currently running thread
	QNode *del = deQueue(runningQueue);		
	free(del->key->thread_context->uc_stack.ss_sp);
	free(del->key->thread_context);
	free(del->key);
	free(del);

	// Switch head ready thread into running queue
	QNode *newRunning = deQueue(readyQueue);
	enQueue(runningQueue, newRunning->key);
	
	// Resume execution of newly running thread
	ucontext_t *newConRef = newRunning->key->thread_context;
	setcontext(newConRef);
}


  
// A utility function to create a new linked list node 
QNode* newNode(tcb *key) 
{ 
	QNode *temp = (QNode*)malloc(sizeof(QNode)); 
	temp->key = key;
	temp->next = NULL; 
	return temp;  
} 
  
// A utility function to create an empty queue 
Queue *createQueue() 
{ 
	Queue *q = (Queue*)malloc(sizeof(Queue)); 
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

// The function to add a thread control block into a queue
void enQueueExisting(Queue *q, QNode *temp) 
{ 
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
	// If queue is empty, return NULL. 
	if (q->front == NULL) 
	   return NULL; 

	// Store previous front and move front one node ahead 
	QNode *temp = q->front; 
	q->front = q->front->next; 

	// If front becomes NULL, then change rear also as NULL 
	if (q->front == NULL) 
	   q->rear = NULL; 
	return temp; 
} 