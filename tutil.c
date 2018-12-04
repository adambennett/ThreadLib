#include "t_lib.h"

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

// Finds and returns first instance of message node with sender = x from a message queue
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

// Finds and returns first instance of thread node with thread_id = tid from a thread queue
QNode *search2(QNode* head, int tid) 
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

/* Function to delete a node in a Doubly Linked List. 
   head_ref --> pointer to head node pointer. 
   del  -->  pointer to node to be deleted. */
void deleteNode(MNode **head_ref, MNode *del, msgQueue *q) 
{ 
  /* base case */
  if (q->front == del)
	  q->front = del->next;

  /* base case 2 */
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

void msgQueuePlumber(msgQueue *q)
{
	mcb *temp;
	MNode *current = q->front;
	
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
	free(q);
}

// Sets up a new message control block with sender/recevier
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

// This function prints contents of linked list starting from the given node 
void printList(QNode* node) 
{ 
    QNode* last; 
    printf("\nTraversal in forward direction \n"); 
    while (node != NULL) 
	{ 
        printf(" %d ", node->key->thread_id); 
        last = node; 
        node = node->next; 
    } 
	printf("\n");
} 

// This function prints contents of linked list starting from the given node 
void printMessages(MNode* node) 
{ 
    MNode* last; 
    printf("\nTraversal in forward direction \n"); 
    while (node != NULL) 
	{ 
        printf(" %s\n", node->key->message); 
        last = node; 
        node = node->next; 
    } 
	printf("\n");
} 