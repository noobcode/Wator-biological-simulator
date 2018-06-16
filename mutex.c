/* \file mutex.c
   \author Carlo Alessi
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  
*/

#include "mutex.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>

static pthread_mutex_t qlock =  PTHREAD_MUTEX_INITIALIZER; /* mutex coda */
static pthread_cond_t  qcond =  PTHREAD_COND_INITIALIZER;  /* CV    coda */
static pthread_mutex_t pwlock = PTHREAD_MUTEX_INITIALIZER; /* mutex pw */
static pthread_mutex_t flock =  PTHREAD_MUTEX_INITIALIZER; /* mutex matrice flag */

static pthread_mutex_t countlock = PTHREAD_MUTEX_INITIALIZER; /* mutex count */
/* CV usata dal collector per aspettare i worker */ 
static pthread_cond_t workers_done = PTHREAD_COND_INITIALIZER; 

static pthread_mutex_t tlock =  PTHREAD_MUTEX_INITIALIZER; /* mutex terminazione */

static pthread_mutex_t chlock = PTHREAD_MUTEX_INITIALIZER; /* mutex pw->chronon */
/* CV usata dal collector per segnalare al dispatcher e ai worker l'inizio di un nuovo chronon */
static pthread_cond_t new_chronon = PTHREAD_COND_INITIALIZER;

static void Pthread_mutex_lock(pthread_mutex_t* mtx){
	int err;
	if( (err = pthread_mutex_lock(mtx)) != 0){
		errno = err;
		perror("lock");
		pthread_exit((void *)errno);
	}
}

static void Pthread_mutex_unlock(pthread_mutex_t* mtx){
	int err;
	if( (err = pthread_mutex_unlock(mtx)) != 0){
		errno = err;
		perror("unlock");
		pthread_exit((void*)errno);
	}
}

void LockQueue(){
	Pthread_mutex_lock(&qlock);
}

void UnlockQueue(){ 
	Pthread_mutex_unlock(&qlock); 
}

void UnlockQueueAndWait(){ 
	pthread_cond_wait(&qcond, &qlock); 
}

void UnlockQueueAndSignal(){
	pthread_cond_signal(&qcond);
    Pthread_mutex_unlock(&qlock);
}

void LockWator(){
	Pthread_mutex_lock(&pwlock);
}

void UnlockWator(){
	Pthread_mutex_unlock(&pwlock);
}

void LockFlag(){
	Pthread_mutex_lock(&flock);
}

void UnlockFlag(){
	Pthread_mutex_unlock(&flock);
}

void LockTerm(){
	Pthread_mutex_lock(&tlock);
}

void UnlockTerm(){
	Pthread_mutex_unlock(&tlock);
}

void LockCount(){
	Pthread_mutex_lock(&countlock);
}

void UnlockCount(){
	Pthread_mutex_unlock(&countlock);
}
 
void waitWorkersDone(){
	pthread_cond_wait(&workers_done, &countlock);
}

void signalWorkersDone(){
	pthread_cond_signal(&workers_done);
}

void signalNewChronon(){
	pthread_cond_signal(&new_chronon);
}

void waitNewChronon(){
	pthread_cond_wait(&new_chronon, &chlock);
}

void LockChronon(){
	Pthread_mutex_lock(&chlock);
}

void UnlockChronon(){
	Pthread_mutex_unlock(&chlock);
}


