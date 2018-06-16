/* \file myqueue.c
   \author Carlo Alessi
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <assert.h>
#include "myqueue.h"
#include "help.h"
#include "mutex.h"
#include <errno.h>

/* alloca ed inizializza un rettangolo
	\param (i,j) ascissa e ordinata del punto in alto a sinistra
	
	le coordinate del secondo punto sono ottenibili con delle operazioni aritmetiche 
	a partire dai parametri i e j.
	
	\retval Rettangolo_t* puntatore al nuovo rettangolo in caso di successo
	\retval NULL in caso di insuccesso	
*/
static Rettangolo_t* newRett(int i, int j){ 
	Rettangolo_t* rett;
	rett = malloc(sizeof(Rettangolo_t));
	CHECK_NULL_PTR(rett, "malloc");
	(rett->a).x = i;
	(rett->a).y = j;
	(rett->b).x = i + K - 1;
	(rett->b).y = j + N - 1;
	rett->next = NULL;
 	return rett;  
}

Queue_t* newQueue(){
	Queue_t* q = malloc(sizeof(Queue_t));
	if(!q){
		perror("malloc");
		return NULL;
	}
	q->head = NULL;
	q->tail = NULL;
	q->qlen = 0;
	return q;
}

void deleteQueue(Queue_t *q) {  
	Rettangolo_t* head = q->head;
    while(head) {
		Rettangolo_t* curr = head;
		head = head->next;
		freeRett(curr);
    }
    free(q);
}

int push(Queue_t** q, int i, int j) {
    Rettangolo_t* rett = newRett(i,j);
    if(!rett)
    	return -1; /* se la creazione del rettangolo è fallita */
    LockQueue();
    if((*q)->head == NULL){
    	(*q)->head = rett;
    	(*q)->tail = rett;
    }else{
    	((*q)->tail)->next = rett;
    	(*q)->tail = rett;
    }
    ((*q)->qlen)++;
    UnlockQueueAndSignal();
    return 0;
}

Rettangolo_t* pop(Queue_t** q, int mywid) {
	Rettangolo_t* rett;        
    LockQueue();
    while((*q)->head == NULL){
		UnlockQueueAndWait();
    }
    /* locked */
    rett = (*q)->head;
    (*q)->head = ((*q)->head)->next;
    (*q)->qlen--;
    assert((*q)->qlen >= 0);
    UnlockQueue();
    return rett;
}

int isEmpty(Queue_t* q){
	int res;
	LockQueue();
	res = (q->qlen == 0);
	UnlockQueue();
	return res;
}

void freeRett(Rettangolo_t* rett){
	free(rett); 
}


