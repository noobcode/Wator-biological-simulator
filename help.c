/* \file help.c
   \author Carlo Alessi
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "help.h"
#include "wator.h"
#include "myqueue.h"
#include <pthread.h>
#include "myconn.h"
#include "mutex.h"

static int terminazione = 0; /* flag per la terminazione del processo wator. */
static int count = 0; /* contatore dei rettangoli aggiornati per il chronon corrente. */

/* thread dispatcher aspetta che il thread collector incrementi il  chronon.
	\param pw puntatore al pianeta
	\param current chronon corrente
*/
static void waitCollector(wator_t* pw, int current);

/* accede in mutua esclusione alla matrice dei flag per vedere se una cella è stata già
   aggiornara per questo chronon
   \param Flag_matrix matrice dei flag
   \param (i,j) posizione della cella
   
	\retval 1 se la cella (i,j) è stata aggiornata
	\retval 0 altrimenti
*/
static int isUpdated(int** Flag_matrix, int i, int j);

/* accede in mutua esclusione ad una cella della matrice
	\param pw puntatore alla struttura
	\param (i,j) posizione della matrice
	
	\retval cell valore della cella	
*/
static cell_t getCell(wator_t* pw, int i, int j);

/* thread collector aspetta che i worker consuminino tutti i task relativi alla elaborazione
   per il chronon corrente.   
	\param q, puntatore alla coda
	\param nRett, numero rettangoli da aggiornare 
*/
static void waitWorkers(Queue_t* q, int nRett);

/* ogni thread worker segnala al collector che è stato aggiornato un nuovo rettangolo,
   potrebbe essere stato l'ultimo.
   ad ogni chiamata viene incrementato un contatore. */
static void signalToCollector();

/* thread collector resetta 'count' */
static void resetCount();

/* data una posizione della matrice dice se è nel bordo del rettangolo oppure no.
	\param (i,j) posizione all'interno della matrice
	
	\retval 1 se è sul bordo, 0 altrimenti 
*/
static int isBorderline(int i, int j);

/* thread collector incrementa il chronon e fa la signal al thread dispatcher 
	\param pw puntatore al pianeta
*/
static void nextChronon(wator_t* pw);

/*-------------------------------------------------------------------*/

int get_coordinata(int a, int incr, int n){		
	if(a == n-1) 					/* a è nel bordo in basso  o a destra */
		return (a + incr) % n;	
	else if(a == 0)					/* a è nel bordo in alto o a sinistra */
		return ((a + incr) + n) % n;
	else
		return (a + incr);			/* a non è nel bordo */
}

void move_or_eat(wator_t* pw, int index, int x, int y, int* k, int* l, coordinate_t* f){
	cell_t temp = pw->plan->w[x][y]; /* pesce o squalo */
	
	if(index == 0){ /* si sposta verso l'alto */
		pw->plan->w[x][y] = WATER;
		pw->plan->w[f->x_up][y] = temp; /* nuova posizione */
		*k = f->x_up;
		*l = y;
	}else if(index == 1){ /* si sposta a destra */
		pw->plan->w[x][y] = WATER;
		pw->plan->w[x][f->y_dx] = temp; /* nuova posizione */
		*k = x;
		*l = f->y_dx;
	}else if(index == 2){ /* si sposta in basso */
		pw->plan->w[x][y] = WATER;
		pw->plan->w[f->x_down][y] = temp; /* nuova posizione */
		*k = f->x_down;
		*l = y;
	}else{
		pw->plan->w[x][y] = WATER;
		pw->plan->w[x][f->y_sx] = temp; /* nuova posizione */
		*k = x;
		*l = f->y_sx;
	}
}

void born(wator_t* pw, int index, int x, int y, int* k, int* l, coordinate_t* f){ 
	cell_t temp = pw->plan->w[x][y]; /* pesce o squalo */
	
	if(index == 0){ /* nasce in alto */
		pw->plan->w[f->x_up][y] = temp; /* pesce o squalo appena nato */
		*k = f->x_up;
		*l = y;
	}else if(index == 1){ /* nasce a destra */
		pw->plan->w[x][f->y_dx] = temp; /* nuova posizione */
		*k = x;
		*l = f->y_dx;
	}else if(index == 2){ /* nasce in basso */
		pw->plan->w[f->x_down][y] = temp; /* nuova posizione */
		*k = f->x_down;
		*l = y;
	}else{			/* nasce a sinistra */
		pw->plan->w[x][f->y_sx] = temp; /* nuova posizione */
		*k = x;
		*l = f->y_sx;
	}
}

void clean(void** a, int nrow){
	int i;
	for(i = 0; i < nrow; i++)
		free(a[i]);
	free(a);	
}

int** alloca_int(int nrow, int ncol){
	int** a;
	int i;
	a = calloc(nrow, sizeof(int*));
	CHECK_NULL_PTR(a, "calloc");
	for(i = 0; i < nrow; i++){	
		a[i] = calloc(ncol, sizeof(int)); /* alloco le righe */
		CHECK_NULL_PTR(a[i], "calloc");		
	}
	assert(a);
	return a;
}

cell_t** alloca_cell_t(int nrow, int ncol){
	cell_t** w;
	int i;
	w = malloc(nrow * sizeof(int*));
	CHECK_NULL_PTR(w, "malloc1");
	for(i = 0; i < nrow; i++){	
		w[i] = malloc(ncol * sizeof(int)); /* alloco le righe */
		CHECK_NULL_PTR(w[i], "malloc2");		
	}
	return w;
}

static cell_t getCell(wator_t* pw, int i, int j){
	cell_t cell;
	LockWator();
	assert(i >= 0);
	assert( i < pw->plan->nrow);
	assert(j >= 0);
	assert(j < pw->plan->ncol);
	cell = pw->plan->w[i][j];
	UnlockWator();
	return cell;
}

static int isUpdated(int** Flag_matrix, int i, int j){
	int result;
	LockFlag();
	result = (Flag_matrix[i][j] == 1);
	UnlockFlag();
	return result;
}

int checkAndSetArgs(int argc, char* argv[], int* worker, int* chronon, char** filename, char** dumpf){
	int worker_flag = 0, chronon_flag = 0, file_flag = 0, dump_flag = 0;
 	int i;
 	int c;
 	char* usage = "wator file [-n worker] [-v chornon] [-f dumpfile]";
 	
  
	/* in caso di errore restituisce il carattere '?' */
  	opterr = 0;
  	
  	/* getopt è chiamata in un loop. quando restituisce -1 non ci sono più opzioni.
	   un secondo loop è usato per i  restanti argomenti non-opzioni 
	*/
	
	/* optind è l'indice dell'array argv del prossimo argomento non-opzione */
	
  	while ((c = getopt (argc, argv, "nvf:")) != -1){
    	switch (c){
      		case 'n':
        		worker_flag++;
        		*worker = atoi(argv[optind]);
        		break;
      		case 'v':
        		chronon_flag++;
        		*chronon = atoi(argv[optind]);
        		break;
      		case 'f':
        		dump_flag++;
        		*dumpf = optarg;
        		break;
      		case '?':
          		fprintf(stderr, "usage: %s.\n", usage);
        		return -1;
      		default:
        		break;
    	}
	}

  	for (i = optind; i < argc; i++){
    	if(!strcmp(FILENAME1, argv[i]) || !strcmp(FILENAME2, argv[i]) || !strcmp(FILENAME3,argv[i])){
    		*filename = argv[i];
    		file_flag++;
    	}
    }
    
    if(file_flag != 1 || worker_flag > 1 || chronon_flag > 1 || dump_flag > 1){
    	return -1;
    }

  	return 0;
}

int setMask(sigset_t* mask){
	/* azzero la maschera e poi metto a 1 i bit corrispondenti ai segnali */
	/* controlla SYSCALL */
	SC_MENO1( sigemptyset(mask),        "sigemptyset");
	SC_MENO1( sigaddset(mask, SIGUSR1), "sigaddset");
	SC_MENO1( sigaddset(mask, SIGINT),  "sigaddset");
	SC_MENO1( sigaddset(mask, SIGTERM), "sigaddset");
	
	/* cambio la signal mask, nel campo 'how' metto SIG_BLOCK: l'insieme dei segnali 
	   bloccati è l'unione della maschera corrente e di 'mask' 
	   NOTA: la maschera viene ereditata da tutti i thread.   
	*/
	if(pthread_sigmask(SIG_BLOCK, mask, NULL) == -1){
		fprintf(stderr, "errore pthread_sigmask\n");
		return EXIT_FAILURE;
	}
	return 0;
}

int updateRett(wator_t* pw, Rettangolo_t* r){
	int i, j;
	int k, l;
	int ris; /* salva il risultato dei valori di ritorno delle regole */
	cell_t cell;
	int border = 0, border_flag = 0; /* salva il risultato della funzione isBorderline */

	for(i = (r->a).x; i < (r->b).x; i++) {
		k = i;
		for(j = (r->a).y; j < (r->b).y; j++) {
			l = j;
			
			/* se è già stata aggiornata non fa nulla */
			if( isUpdated(Flag_matrix, i, j) ) continue; 	
			
			border = isBorderline(i, j);
			cell = getCell(pw, i, j);
			 
			switch(cell) {
				case FISH:
					if(border) 
						LockWator();	
					ris = fish_rule4(pw,i,j,&k,&l);
					if(ris == -1) {
						fprintf(stderr,"errore fish_rule4\n");
						return -1;
					}
					if(k != i || l != j) {
						/* è nato un pesce */
						border_flag = isBorderline(k,l);
						if(border_flag) LockFlag();
						Flag_matrix[k][l] = 1; 
						if(border_flag) UnlockFlag();
					}
					k = i; l = j;
					ris = fish_rule3(pw, i, j, &k, &l);
					if(ris == -1){
						fprintf(stderr, "errore fish_rule3\n");
						return -1;
					} else if(ris == MOVE){
				 		/* setta la nuova posizione del pesce a 1 */
				 		border_flag = isBorderline(k,l);
				 		if(border_flag) LockFlag();
						Flag_matrix[k][l] = 1;
						if(border_flag) UnlockFlag();
					}
					if(border) UnlockWator();
					break;
				case SHARK:
					if(border) LockWator();
					ris = shark_rule2(pw,i,j,&k,&l);
					if(ris == -1){
						fprintf(stderr, "errore shark_rule2\n");
						return -1;
					} else if(ris == ALIVE) {
						if(k != i || l != j) {
							/* ha fatto un figlio */
							border_flag = isBorderline(k,l);
							if(border_flag) LockFlag(); 
							Flag_matrix[k][l] = 1;
							if(border_flag) UnlockFlag();
						}
						k = i; l = j;
						ris = shark_rule1(pw,i,j,&k,&l);
						if(ris == -1){
							fprintf(stderr, "errore shark_rule1\n");
							return -1;
						}
						if(ris == MOVE || ris == EAT) {
							/* se si è mosso oppure ha mangiato */
							border_flag = isBorderline(k,l);	 
							if(border_flag) LockFlag();
							Flag_matrix[k][l] = 1;
							if(border_flag) UnlockFlag();	
						}
					}
					if(border) UnlockWator();
					break;
				case WATER:
					break;
			} /* fine switch */
		} /* fine for annidato */
	} /* fine for */
	return 0;
}

int terminate(){
	int ris;
	LockTerm();
	ris = terminazione;
	UnlockTerm();
	return ris;
}

void setTerminate(){
	LockTerm();
	terminazione = 1;
	UnlockTerm();
}

int getChronon(wator_t* pw){
	int res;
	LockWator();
	res = pw->chronon;
	UnlockWator();
	return res;
}

int init_worker(pthread_t** worker,thWorkerArgs** workerArgs, wator_t* pw, Queue_t* q){
	int i;
	*worker 	= malloc(pw->nwork * sizeof(pthread_t));
	*workerArgs = malloc(pw->nwork * sizeof(thWorkerArgs));
	CHECK_MENO_1(*worker, "malloc3");
	CHECK_MENO_1(*workerArgs, "malloc4");
	/* inizializzo parametri dei worker */
	for(i = 0; i < pw->nwork; i++){
		(*workerArgs)[i].wid = i;
		(*workerArgs)[i].pw = pw;
		(*workerArgs)[i].q = q;
	}
	return 0;
}

void init_dispatcher(thDispatcherArgs* d, Queue_t* q, wator_t* pw){
	d->q = q;
	d->pw = pw;
	d->nrow = pw->plan->nrow;
	d->ncol = pw->plan->ncol;
}

void init_collector(thCollectorArgs* c, int nchron, wator_t* pw, Queue_t* q){
	c->pw = pw;
	c->nchron = nchron;
	c->q = q;
}

static void waitCollector(wator_t* pw, int current){
	LockChronon(); 
	while(current == pw->chronon ){
		waitNewChronon();
	}
	UnlockChronon(); 
}

static void nextChronon(wator_t* pw){
	LockChronon();
	(pw->chronon)++;
	signalNewChronon();
	UnlockChronon();	
}

void* thWorker(void* arg){
	FILE* file_worker;
	int mywid   = ((thWorkerArgs*)arg)->wid;
	/* wator_t* pw = ((thWorkerArgs*)arg)->pw; */
	Queue_t* q  = ((thWorkerArgs*)arg)->q;
	char filename[15] = "wator_worker_"; /* assumendo wid = [0-9] bastano 15 char */
	
	sprintf(filename, "wator_worker_%d", mywid); /* mette la stringa nel buffer */	
	filename[strlen(filename)] = '\0';
	
	file_worker = fopen(filename, "w+");
	if(file_worker == NULL){
		perror("fopen");
		pthread_exit((void*)1);
	}
		
	while(1)
	{
		Rettangolo_t* rett = pop(&q, mywid);	
		assert(rett);	
		if((rett->a).x == -20)
		{
			/* rettangolo terminazione */ 
			freeRett(rett);
			/*signalToCollector();*/
			fclose(file_worker);
			pthread_exit((void*)0);
		} else {  
			/* calcola l'aggiornamento */
			/*
			int res;
			res = updateRett(pw, rett);
			if(res == -1){
				fprintf(stderr, "errore: updateRett\n");
				pthread_exit((void*)1);
			}
			*/
			freeRett(rett);
			signalToCollector();
		}
	}	
}

void* thDispatcher(void* arg){
	Queue_t* q =  ((thDispatcherArgs*)arg)->q;
	wator_t* pw = ((thDispatcherArgs*)arg)->pw;
	int nrow =    ((thDispatcherArgs*)arg)->nrow;
	int ncol =    ((thDispatcherArgs*)arg)->ncol;
	int i = 0, j = 0;
	int ris;
	int current; /* chronon corrente */
	
	while(1)
	{
		current = getChronon(pw);
		/* genera i rettangoli e li inserisce nella queue */
		while(i < nrow){
			while(j < ncol){
				ris = push(&q, i, j);
				if(ris == -1){
					fprintf(stderr, "errore della push\n");
					pthread_exit((void*)1);
				}
				j = j + N;
			}		
			j = 0;
			i = i + K;
		}
		i = 0;
		j = 0;
		
		waitCollector(pw, current);
		assert(q->head == NULL);
	}	
	pthread_exit((void*)0);	
}

void* thCollector(void* arg){
	wator_t* pw = ((thCollectorArgs*)arg)->pw;
	int nchron =  ((thCollectorArgs*)arg)->nchron;	
	Queue_t* q =  ((thCollectorArgs*)arg)->q;
	int nRett = (pw->plan->nrow * pw->plan->ncol) / (K * N);	/* gestisci matrici diverse */
	int res;
	
	while(!terminate())
	{		
		if(getChronon(pw) % nchron == 0){
			res = connectToVisualizer(pw, 0); /* 0 flag non terminazione */
			if(res == -1){
				fprintf(stderr, "connessione al visualizer non riuscita\n"); 
				pthread_exit((void*)1);
			}		
		}
		waitWorkers(q, nRett);
		resetCount();
		nextChronon(pw); 
	}
	assert(terminate() == 1);
	pthread_exit((void*)0);
}

void print_status(int pid, int status){
	if( WIFEXITED(status)){
		/* terminazione normale */
		fprintf(stderr, "visualizer -- exit value: %d\n", WEXITSTATUS(status));
	}
	if( WIFSIGNALED(status)){
		/* segnale */
		fprintf(stderr, "visualizer -- killed by signal: %d\n", WTERMSIG(status));
	}
	if( WCOREDUMP(status)){
		/* core file */
		fprintf(stderr, "visualizer -- core dumped\n");
	}
	if( WIFSTOPPED(status)){
		/* stoppato */
		fprintf(stderr, "visualizer -- stopped by signal: %d\n", WSTOPSIG(status));
	}
}

static void waitWorkers(Queue_t* q, int nRett){
	LockCount();
	/* finché non sono stati aggiornati tutti i rettangoli */
	while(count != nRett){
		waitWorkersDone();	
	}
	assert(count == nRett);
	UnlockCount();			
}

int getCount(){
	int result;
	LockCount();
	result = count;
	UnlockCount();
	return result;
}

static void resetCount(){
	LockCount();
	count = 0;
	UnlockCount();
}

static void signalToCollector(){
	/* incrementa contatore */
	LockCount();
	count++;
	signalWorkersDone();
	UnlockCount();
}

static int isBorderline(int i, int j){
	int a = i % K;
	int b = j % N;
	return (a == 4 || a == 0) || (b == 4 || b == 0);
}
