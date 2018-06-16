/* \file help.h
   \author Carlo Alessi
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  
*/
#ifndef HELP_H
#define HELP_H

#include "wator.h"
#include "myqueue.h"
#include "myconn.h"

#define CHECK_NULL_PTR(p, str) if(!(p)) { perror(str); return NULL; } 
#define CHECK_MENO_1(p,str)    if(!(p)) { perror(str); return -1; }
#define SYS_CALL(p, str)       if((p) == -1) { perror(str); exit(EXIT_FAILURE); }		
#define CHECK_NULL(p,str)      if(!(p)) { perror(str); exit(EXIT_FAILURE); }	

/* se la SC ritorna -1 allora restituisco -1 */
#define SC_MENO1(p, str)       if(p == -1) { perror(str); return -1; }

#define CHECK(p, str) if((p) != 0) { fprintf(stderr, str); exit(EXIT_FAILURE); }

#define FILENAME1 "planet1.dat"
#define FILENAME2 "planet2.dat"
#define FILENAME3 "planet3.dat"

#define NWORK_DEF 3
#define CHRON_DEF 25

int** Flag_matrix; /* matrice di flag usata per l'aggiornamento */

/* struttura usata per memorizzare le coordinate effettive */
typedef struct coordinate{
	int x_up;	/* x sopra */
	int x_down; /* x in basso */
	int y_sx;	/* y a sinistra */
	int y_dx;	/* y a destra */
} coordinate_t;

/* parametri di un thread worker */
typedef struct thWorkerArgs{
	int wid; 	 /* worker identifier */
	wator_t* pw; /* puntatore al pianeta */
	Queue_t* q;  /* puntatore alla coda condivisa */
} thWorkerArgs;

typedef struct thDispatcherArgs{
	Queue_t* q;   /* puntatore alla Queue */
	wator_t* pw;
	int nrow;
	int ncol;
} thDispatcherArgs;

typedef struct thCollectorArgs{
	wator_t* pw; /* puntatore al pianeta */
	int nchron;  /* ogni quanti chronon deve essere effettuata la simulazione */
	Queue_t* q;  /* puntatore alla coda condivisa */
} thCollectorArgs;

/*	ottiene la coordinata adiacente a quella attuale tenendo presente che
	la matrice corrisponde ad un pianeta rotondo.
	
	\param a può essere la x oppure la y
	\param incr è l'incremento della coordinata, può essere +-1
	\param n può essere il numero di righe o il numero di colonne
	
	x +1 -> down
	x -1 -> up
	y +1 -> dx
	y -1 -> sx
	
	\retval la coordinata effettiva
*/
int get_coordinata(int a, int incr, int n);

/*  gli squali mangiano e si spostano, i pesci si spostano.
	la funzione è chiamata sia in shark_rule1 che in fish_rule3.
	
	\param pw puntatore alla struttura di simulazione
	\param (x,y) coordinate iniziali del pesce / squalo
    \param (*k,*l) coordinate finali del pesce / squalo
	\param index (valore da 0 a 3)
	\param *f coordinate adiacenti nel mondo
	
	in base al valore di index lo squalo/pesce si sposta nel modo seguente:
	index == 0 -> up
	index == 1 -> dx
	index == 2 -> down
	index == 3 -> sx

	\retval void
*/
void move_or_eat(wator_t* pw, int index, int x, int y, int* k, int* l, coordinate_t* f);

/* fa nascere un pesce oppure uno squalo. 
	\param pw puntatore alla struttura di simulazione
	\param (x,y) coordinate del pesce / squalo 
	\param (*k,*l) coordinate del pesce / squalo appena generato
	\param *f coordinate adiacenti nel mondo
	
	index ha lo stesso funzionamento che ha nella funzione move_or_eat()

	\retval void
*/
void born(wator_t* pw, int index, int x, int y, int* k, int* l, coordinate_t* f);

/* libera la memoria.
	\param a matrice di qualsiasi tipo
	\param nrow numero righe
*/
void clean(void** a, int nrow);

/* alloca matrice di interi 
	\param nrow numero righe
	\param ncol numero colonne
	
	gli elementi della matrice vengono inizializzati a 0.
	
	\retval a matrice di interi 
	\retval NULL in caso di errore
*/
int** alloca_int(int nrow, int ncol);

/* alloca matrice di celle cell_t.
	\param nrow numero righe
	\param ncol numero colonne
	
	\retval matrice di celle
*/
cell_t** alloca_cell_t(int nrow, int ncol);

/*	controlla gli argomenti passati da linea di comando al processo wator.
	\param argc numero argomenti
	\param argv[] vettore degli argomenti
	\param int* worker
	\param int* chronon
	\param char** filename , punterà al nome del file specificato
	\param char** dumpf, punterà al nome del file di dump specificato
	
	il numero di worker e chronon, se specificate le opzioni -v e -n, vengono inizializzati.
	filename punterà al nome del file che rappresenta il pianeta.
	dumpf, se viene specificata l'opzione -f, punterà al nome del file sul quale il
	processo visualizer effettuerà il dump.
	
	\retval -1 in caso di errore
	\retval 0 in caso di successo
*/
int checkAndSetArgs(int argc, char* argv[], int* worker, int* chronon, char** filename, char** dumpf);

/* setta la maschera 'mask' bloccando i segnali SIGUSR1, SIGINT e SIGTERM
	\param mask, puntatore a una maschera
	
	\retaval 0 in caso di successo
	\retval -1 in caso di errore
*/
int setMask(sigset_t* mask);

/* inizializza la struttura che rappresenta i parametri dei thread worker.
	\param pthread** worker, indirizzo di un array di worker
	\param thWorkerArgs** workerArgs, indirizzo di un array di struct
	\param wator* pw 
	
	\retval 0 in caso di successo, -1 altrimenti

*/
int init_worker(pthread_t** worker, thWorkerArgs** workerArgs, wator_t* pw, Queue_t* q);

/* procedura eseguita dai vari thread Worker
	\param struct thWorkerArgs
	
	si comporta come previsto nelle specifiche.
	
	\retval (void*) 0 in caso di successo
	\retval (void*) 1 in caso di errore
	
*/
void* thWorker(void* arg);

/* procedura eseguita dal thread Dispatcher
	\param struct thDispatcherArgs
	
	si comporta come previsto nelle specifiche.
	
	\retval (void*) 0 in caso di successo
	\retval (void*) 1 in caso di errore
	
*/
void* thDispatcher(void* arg);

/* procedura eseguita dal thread Collector
	\param struct thCollectorArgs
	
	si comporta come previsto nelle specifiche.
	
	\retval (void*) 0 in caso di successo
	\retval (void*) 1 in caso di errore
*/
void* thCollector(void* arg);

/* inizializza la struttura che rappresenta i parametri del thread dispatcher
	\param thDispatcherArgs* d, puntatore alla struttura dati dei parametri
	\param Queue_t* q, puntatore alla coda condivisa
	\param wator_t* pw puntatore al pianeta
	
	\retval void
*/
void init_dispatcher(thDispatcherArgs* d, Queue_t* q, wator_t* pw);

/* inizializza la struttura che rappresenta i parametri del thread collector
	\param thCollectorArgs* c, puntatore alla struttura dati dei parametri
	\param Queue_t* q, puntatore alla coda condivisa
	\param wator_t* pw puntatore al pianeta
	\param int nchron, ogni quanti chronon deve essere effettuata la visualizzazione
	
	\retval void
*/
void init_collector(thCollectorArgs* c, int nchron, wator_t* pw, Queue_t* q);


/* accede in mutua esclusione alla condizione di terminazione dei thread
	\retval 1 se terminazione == 1;
	\retval 0 se terminazione == 0;
*/
int terminate();

/* accede in mutua esclusione e setta la variabile 'terminazione' a 1 */
void setTerminate();

/* accede in mutua esclusione alla struttura di simulazione e restituisce il chronon corrente.
	\param pw puntatore al pianeta
	
	\result pw->chronon
*/
int getChronon(wator_t* pw);


/* stampa l'exit status del processo visualizer.
	\param pid, process identifier
	\param status, exit status
*/
void print_status(int pid, int status);

/* ottiene il valore di 'count'.
	\retval result, valore del contatore
*/
int getCount();

/* calcola un chronon aggiornando tutti i valori della simulazione e il 'rettangolo'
   \param pw puntatore al pianeta
   \param r rettangolo (sottomatrice) da aggiornare
   
   \return 0 se tutto e' andato bene
   \return -1 se si e' verificato un errore
*/
int updateRett(wator_t* pw, Rettangolo_t* r);

#endif
