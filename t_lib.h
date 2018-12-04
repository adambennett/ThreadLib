#ifndef tlib_h
#define tlib_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <sys/mman.h>

#include "tstruct.h"
#include "tutil.h"

void t_yield();
void t_init();
void t_shutdown();
void t_terminate();
void t_create(void (*fct)(int), int id, int pri);
void sem_wait(sem_t *s);
void sem_signal(sem_t *s);
void sem_destroy(sem_t **s);
int sem_init(sem_t **s, int sem_count);
int mbox_create(mbox **mb);
void mbox_destroy(mbox **mb);
void mbox_deposit(mbox *mb, char *msg, int len);
void mbox_withdraw(mbox *mb, char *msg, int *len);
void send(int tid, char *msg, int len);
void receive(int *tid, char *msg, int *len);

#endif //tlib_h