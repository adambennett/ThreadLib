#ifndef tutil_h
#define tutil_h

void enQueue(Queue *q, tcb *tcb);
void enQueueMsg(msgQueue *q, mcb *mcb);
QNode *newNode(tcb *key);
MNode *newMsgNode(mcb *mcb);
QNode *deQueue(Queue *q);
MNode *deQueueMsg(msgQueue *q);
Queue *createQueue();
msgQueue *createMsgQueue();
void deleteNode(MNode **head_ref, MNode *del, msgQueue *q);
MNode *search(MNode* head, int x);
QNode *search2(QNode* head, int tid);
void printList(QNode* node);
void printMessages(MNode* node);
mcb *newMsg(char *msg, int len, int sender, int rec);
void msgQueuePlumber(msgQueue *q);

#endif //tutil.h