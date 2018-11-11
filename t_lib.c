#include "t_lib.h"

ucontext_t *running;
//Queue *readyQueue;
//Queue *runningQueue;
//Queue *lowReadyQueue;
priorityQueue *ready;
QNode *runThread;

void t_yield()
{
	ualarm(0,0);
	printList(ready);
	QNode *next = pop(ready);
	if (next != NULL)
	{
		QNode *current = runThread;
		next->next = NULL;
		current->next = NULL;
		push(ready, current);
		runThread = next;
		//printList(ready);
		init_alarm();
		swapcontext(current->key->thread_context, next->key->thread_context);
		//printList(ready);
		//printf("--\n");
	}
	else
	{
		init_alarm();
	}


	/*
	QNode *prevRunning = deQueue(runningQueue);
	QNode *prevReady = deQueue(readyQueue);

	// High priority threads are available
	if (prevReady != NULL)
	{
		enQueue(runningQueue, prevReady->key);
		if (prevRunning->key->thread_priority == 0) { enQueue(readyQueue, prevRunning->key); }
		else { enQueue(lowReadyQueue, prevRunning->key); }
		running = prevReady->key->thread_context;
		init_alarm();
		swapcontext(prevRunning->key->thread_context, running);
		free(prevReady);
		free(prevRunning);
	}
	
	// Only low priority threads are available
	else
	{
		// Running thread is high priority
		if (prevRunning->key->thread_priority == 0)
		{
			enQueue(runningQueue, prevRunning->key);
			free(prevRunning);
			init_alarm();
		}
		
		// Running and all ready threads are low priority
		else
		{
			prevReady = deQueue(lowReadyQueue);
			if (prevReady != NULL)
			{
				enQueue(runningQueue, prevReady->key);
				enQueue(lowReadyQueue, prevRunning->key);
				running = prevReady->key->thread_context;
				init_alarm();
				swapcontext(prevRunning->key->thread_context, running);
				free(prevReady);
				free(prevRunning);
			}
		}
	}
	*/
}

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
	main->thread_priority = 1;
	main->thread_context = tmp;

	// Initialize running and ready queues
	//lowReadyQueue = createQueue();
	//readyQueue = createQueue(); 
	//runningQueue = createQueue(); 
	runThread = newNode(main);
	ready = createPriQueue();
	
	// Insert main thread into running queue
	//enQueue(runningQueue, main);
	
	init_alarm();

}

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
	//uc->uc_link = runThread->key->thread_context;
	makecontext(uc, (void (*)(void)) fct, 1, id);

	tcb *thread = (tcb *) malloc(sizeof(tcb));
	thread->thread_id = id;
	thread->thread_priority = pri;
	thread->thread_context = uc;
	//if (pri == 0) {	enQueue(readyQueue, thread); }
	//else { enQueue(lowReadyQueue, thread); }
	
	ualarm(0,0);
	QNode *temp = newNode(thread);
	push(ready, temp);
	init_alarm();
}

void t_shutdown()
{
	QNode *temp = ready->front;
	if (temp != NULL)
	{
		while (temp->next != NULL)
		{
			QNode *current = temp;
			temp = temp->next;
			if (temp->key->thread_id > 0) { free(temp->key->thread_context->uc_stack.ss_sp); }
			
			free(temp->key->thread_context);
			free(temp->key);
			free(temp);
		}
	}
	//QNode *current = readyQueue->front;
	//QNode *current2 = lowReadyQueue->front;
	
	/*
	// Free readyQueue data - QNodes, TCBs, and context references
	while (current != NULL)
	{
		temp = current;
		if (current->next != NULL) { current = current->next; }
		else { current = NULL; }
		
		if (temp->key->thread_id > 0) { free(temp->key->thread_context->uc_stack.ss_sp); }
		
		free(temp->key->thread_context);
		free(temp->key);
		free(temp);
	}
	
	// Free lowReadyQueue data - QNodes, TCBs, and context references
	while (current2 != NULL)
	{
		temp = current2;
		if (current2->next != NULL) { current2 = current2->next; }
		else { current2 = NULL; }
		
		if (temp->key->thread_id > 0) { free(temp->key->thread_context->uc_stack.ss_sp); }
		
		free(temp->key->thread_context);
		free(temp->key);
		free(temp);
	}
	*/

	// Free runningQueue data - QNode, TCB, and context reference
	//temp = runningQueue->front;
	temp = runThread;
	free(temp->key->thread_context);
	free(temp->key);
	free(temp);
	
	// Free queue structs
	//free(readyQueue);
	//free(runningQueue);
	free(ready);
}

void t_terminate()
{
	QNode *temp = runThread;
	free(temp->key->thread_context->uc_stack.ss_sp);
	free(temp->key->thread_context);
	free(temp->key);
	free(temp);
	
	runThread = pop(ready);
	if (runThread != NULL)
	{
		setcontext(runThread->key->thread_context);
	}
	
	/*
	// Dequeue currently running thread
	QNode *del = deQueue(runningQueue);
	free(del->key->thread_context->uc_stack.ss_sp);
	free(del->key->thread_context);
	free(del->key);
	free(del);
	

	// Switch head ready thread into running queue
	QNode *newRunning = deQueue(readyQueue);
		// free newRunning before deQueing next
	if (newRunning == NULL) { newRunning = deQueue(lowReadyQueue); }
	enQueue(runningQueue, newRunning->key);
	free(newRunning);
	*/
	
	// Resume execution of newly running thread
	//ucontext_t *newConRef = newRunning->key->thread_context;
	//setcontext(newConRef);
}

void sig_hand(int sig_no)
{
	t_yield();
}

void init_alarm()
{
	signal(SIGALRM, sig_hand);
	ualarm(10000, 0);
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

// A utility function to create an empty priorityQueue 
priorityQueue *createPriQueue() 
{ 
	priorityQueue *q = (priorityQueue*)malloc(sizeof(priorityQueue)); 
	q->front = NULL; 
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

void push(priorityQueue *q, QNode *node)
{
	node->next = NULL;
	QNode *current = q->front;
	
	// If empty queue
	if (current == NULL)
	{
		q->front = node;
	}
	
	// Populated queue
	else
	{
		// Check node priority and insert appropriately in queue
		if (node->key->thread_priority < q->front->key->thread_priority)
		{
			node->next = q->front;
			q->front = node;
		}
		else
		{
			while (current->next != NULL && current->next->key->thread_priority < node->key->thread_priority)
			{
				current = current->next;
			}
			current->next = node;
		}
	}
}

QNode *pop(priorityQueue *q)
{
	QNode *temp = NULL;
	if (q->front != NULL)
	{
		temp = q->front;
		q->front = q->front->next;
		temp->next = NULL;
	}
	
	if (temp != NULL)
	{
		temp->next = NULL;
	}
	
	return temp;
}

void printList(priorityQueue *queueHead) 
{
	QNode *currentNode;
	printf("[%d] ", runThread->key->thread_id);
	if (NULL != queueHead) 
	{
		printf("{");
		currentNode = queueHead->front;
		while (NULL != currentNode) 
		{
			printf("%d, ", currentNode->key->thread_id);
			currentNode = currentNode->next;
		}
		printf("}");
	} 
	else 
	{
		printf("{ }");
	}
	printf("\n");
}