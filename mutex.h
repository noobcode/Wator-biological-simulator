/* \file mutex.c
   \author Carlo Alessi
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  
*/

#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>

/* ridefinizione di pthread_mutex_lock con l'aggiunta dei controlli errori */
/*static void Pthread_mutex_lock(pthread_mutex_t* mtx);*/

/* ridefinizione di pthread_mutex_unlock con l'aggiunta dei controlli errori */
/*static void Pthread_mutex_unlock(pthread_mutex_t* mtx);*/

/* blocca l'accesso alla coda */
void LockQueue();

/* sblocca l'accesso alla coda */
void UnlockQueue();

/* atomicamente lascia il lock e si mette in attesa della CV. */
void UnlockQueueAndWait();

/* segnala l'evento della CV e rilascia il lock */
void UnlockQueueAndSignal();

/* blocca l'accesso al pianeta */
void LockWator();

/* sblocca l'accesso al pianeta */
void UnlockWator();

/* blocca accesso alla matrice dei flag */
void LockFlag();

/* sblocca l'accesso alla matrice dei flag */
void UnlockFlag();

/* blocca l'accesso alla variabile 'terminazione' */
void LockTerm();

/* sblocca accesso alla variabile 'terminazione' */
void UnlockTerm();

/* blocca l'accesso al contatore */
void LockCount();

/* sblocca l'accesso al contatore */
void UnlockCount();

/* atomicamente si mette in attesa della CV workers_done e rilascia il lock del contatore dei rettangoli aggiornati.*/
void waitWorkersDone();

/* fa una signal alla CV workers_done */ 
void signalWorkersDone();

/* fa una signal alla CV new_chronon per segnalare che è avvenuto l'aggiornamento del chronon */
void signalNewChronon();

/* atomicamente si mette in attesa della CV new_chronon e rilascia il lock 'pwlock' */
void waitNewChronon();

/* blocca l'accesso alla variabile pw->chronon */
void LockChronon();

/* sblocca l'accesso alla variabile pw->chronon */
void UnlockChronon();

#endif
