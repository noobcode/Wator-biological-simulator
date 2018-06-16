/* \file myqueue.h
   \author Carlo Alessi
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  
*/

#ifndef QUEUE_H_
#define QUEUE_H_

#define K 5 /* altezza del rettangolo */
#define N 5 /* lachezza del rettangolo */

typedef struct Point{
	int x;	/* ascissa del punto */
	int y;	/* ordinata del punto */
} Point_t;

typedef struct Rettangolo{
	Point_t a; /* estremo in alto a sx del rettangolo */
	Point_t b; /* estremo in basso a dx del rettangolo */
	struct Rettangolo* next;
} Rettangolo_t;

typedef struct Queue{
	Rettangolo_t* head;
	Rettangolo_t* tail;
	int qlen;
} Queue_t;

/* alloca spazio per una coda. 
	\retval puntatore alla queue allocata
*/
Queue_t* newQueue();

/* Cancella una coda allocata con newQueue.
    \param q puntatore alla coda da cancellare
*/

void deleteQueue(Queue_t* q);

/* crea e inserisce un rettangolo in coda nella Queue q.
	\param q indirizzo del puntatore alla coda
	\param (i,j) primo punto del rettangolo
	
	invoca al suo interno la newRett(i,j).
  
	\retval 0 se successo
    \retval -1 se errore
*/
int push(Queue_t** q, int i, int j); 

/* estrae un rettangolo dalla testa della coda.
 	\param q, indirizzo del puntatore alla coda.
 	
	\retval rett puntatore al rettangolo estratto.
*/
Rettangolo_t* pop(Queue_t** q, int mywid);

/* libera la memoria allocata da un rettangolo */
void freeRett(Rettangolo_t* r);

/* blocca l'accesso al rettangolo */
/*void LockRett(Rettangolo_t* r);*/

/* sblocca l'accesso al rettangolo */
/*void UnlockRett(Rettangolo_t* r);*/

/* testa se la coda è vuota.
	\param q puntatore alla coda
	
	\retval 1 se la coda è vuota
	\retval 0 altrimenti
*/
int isEmpty(Queue_t* q);



#endif /* QUEUE_H_ */
