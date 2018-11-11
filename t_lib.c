#include "t_lib.h"

ucontext_t *running;
priorityQueue *ready;
QNode *runThread;

void t_yield()
{
	
	ualarm(0,0);
	QNode *next = pop(ready);
	
	if (next != NULL)
	{
		QNode *current = runThread;
		next->next = NULL;
		current->next = NULL;
		push(ready, current);
		runThread = next;
		init_alarm();
		swapcontext(current->key->thread_context, next->key->thread_context);
	}
	else
	{
		init_alarm();
	}

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

	// Initialize running and ready queue
	runThread = newNode(main);
	ready = createPriQueue();
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
	makecontext(uc, (void (*)(void)) fct, 1, id);

	tcb *thread = (tcb *) malloc(sizeof(tcb));
	thread->thread_id = id;
	thread->thread_priority = pri;
	thread->thread_context = uc;
	
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
	
	// Free runningQueue data - QNode, TCB, and context reference
	//temp = runningQueue->front;
	//temp = runThread;
	//free(temp->key->thread_context);
	//free(temp->key);
	//free(temp);
	
	// Free queue structs
	//free(readyQueue);
	//free(runningQueue);
	//free(ready);
}

void t_terminate()
{
	QNode *temp = runThread;
	
	runThread = pop(ready);
	if (runThread != NULL)
	{
		setcontext(runThread->key->thread_context);
	}
	
	free(temp->key->thread_context->uc_stack.ss_sp);
	free(temp->key->thread_context);
	free(temp->key);
	free(temp);
	
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
			bool pushed = false;
			while (current->next != NULL && pushed == false)
			{
				// If node is higher priority than any other element encountered
				if (node->key->thread_priority < current->next->key->thread_priority)
				{
					node->next = current->next;
					current->next = node;
					pushed = true;
				}
				
				// Node has equal or lower priority than encountered element so keep looping through until the end
				else
				{
					current = current->next;
				}
			}
			
			if (pushed == false)
			{
				current->next = node;
			}
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