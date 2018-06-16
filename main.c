/* \file main.c
   \author Carlo Alessi
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "wator.h"
#include "help.h"
#include "myqueue.h"
#include "myconn.h"
#include "mutex.h"

int main(int argc, char* argv[]){
	int nwork, sig, status;
	int nchron; /* ogni quanti chronon lo stato del pianeta deve essere visualizzato */
	char* filename = NULL, *dumpfile = NULL;
	FILE* f_check; /* wator.check */
	sigset_t mask;
	wator_t* pw;
	int pid_visualizer, err, i; /* test per errori, indice */
	pthread_t *worker = NULL, collector, disp;
	thWorkerArgs* workerArgs = NULL; 
	thCollectorArgs collectorArgs;
	thDispatcherArgs dispatcherArgs;
	Queue_t* q;
	void* res; /* codice di ritorno del thread Dispatcher */
	
	/* assegnazione di dafault */
	nwork =  NWORK_DEF;
	nchron = CHRON_DEF;
	
	/* controllo formato argomenti e assegna i valori */
	err = checkAndSetArgs(argc, argv, &nwork, &nchron, &filename, &dumpfile);
	if(err == -1){
		fprintf(stderr, "errore: controllo formato argomenti non riuscito\n");	
		return EXIT_FAILURE;
	 }
	assert(filename);
	
	/* setta la maschera */
	err = setMask(&mask);
	if(err == -1){
		fprintf(stderr, "errore: settaggio della maschra non riusciuto\n");
		return EXIT_FAILURE;
	}
	
	/* carica da file la matrice che rappresenta il pianeta */
	pw = new_wator(filename);	assert(pw);
	pw->nwork = nwork;
	CHECK_NULL(f_check = fopen("./wator.check", "w"), "fopen_check");
	Flag_matrix = alloca_int(pw->plan->nrow, pw->plan->ncol);
	assert(Flag_matrix);
	/* alloco la Queue */
	q = newQueue();	
	
	/* inizializza i parametri dei thread */
	if( init_worker(&worker, &workerArgs, pw, q) == -1){
		fprintf(stderr, "errore: inizializzazione workers non riuscita\n");
		return EXIT_FAILURE;
	}
	init_dispatcher(&dispatcherArgs, q, pw);
	init_collector(&collectorArgs, nchron, pw, q);
	
	/* attiva il processo visualizer */
	SYS_CALL(pid_visualizer = fork(), "fork");
	if(pid_visualizer == 0){
		/* processo figlio */
		execl("./visualizer", "visualizer", dumpfile, (char*) NULL);
		perror("execl");
		exit(EXIT_FAILURE);
	}
	/* processo padre */
	
	/* lancio il thread dispatcher */
	CHECK( pthread_create(&disp, NULL, thDispatcher, (void*)&dispatcherArgs), "pthread_create");
	/* lancio il thread collector */
	CHECK( pthread_create(&collector, NULL, &thCollector, (void*)&collectorArgs), "pthread_create");
	/* lancio i thread worker */
	for(i = 0; i < nwork; i++)
		CHECK( pthread_create(&worker[i], NULL, &thWorker, (void*)&workerArgs[i]), "pthread_create");

	while(1){
		/* si mette in attessa di un segnale */
		err = sigwait(&mask, &sig);
		if(err != 0){
	    	errno = err;
	    	perror("sigwait");
	    	exit(EXIT_FAILURE);	
	    }
		/* controllo quale segnale ho ricevuto */
		switch(sig){
			case SIGUSR1:
				fprintf(stdout,"ricevuto SIGUSR1\n");
				fflush(stdout);
				print_planet(f_check, pw->plan); /* checkpoint */
				break;
			case SIGINT:
			case SIGTERM:
				fprintf(stdout, "ricevuto SIGTERM o SIGINT\n");
				fflush(stdout);
				setTerminate();
				assert(terminate() == 1);
				break;
			default:
				fprintf(stderr, "errore: ricevuto segnale %d\n", sig);
				abort();
		}
		if(terminate()) break;
	}
	assert(terminate() == 1);

	/* attendo terminazione del thread collector */
	CHECK( pthread_join(collector, NULL), "pthread_join\n");	
	printf("thread collector terminato\n");
	
	/* mando una richiesta di cancellazione */
	CHECK( pthread_cancel(disp), "pthread_cancel\n");
	/* attendo terminazione dispatcher */
	CHECK(pthread_join(disp, &res), "pthread_join\n");
	if (res != PTHREAD_CANCELED){
    	fprintf(stderr, "errore: thread dispatcher non cancellato\n");
		return EXIT_FAILURE;
	}
	printf("thread dispatcher terminato\n");
	
	/* mando End Of Stream per terminare i thread worker */
	for(i = 0; i < nwork; i++){
		err = push(&q, -20, -20);
		if(err == -1){
			fprintf(stderr, "errore della push\n");
			return EXIT_FAILURE;
		}
	}
	
	/* attendo terminazione dei thread worker */
	for(i = 0; i < nwork; i++)
		CHECK( pthread_join(worker[i], NULL), "pthread_join\n");
	printf("thread worker terminati\n");
	
	/* terminazione del processo visualizer */
	err = connectToVisualizer(pw, 1); /* 1 flag per la terminazione */
	if(err == -1){
		fprintf(stderr, "errore terminazione visualizer\n");
		return EXIT_FAILURE;
	}
	SYS_CALL( waitpid(pid_visualizer, &status, 0), "waitpid");
	print_status(pid_visualizer, status);
		
	clean((void**)Flag_matrix, pw->plan->nrow);
	free_wator(pw);
	free(worker);
	fclose(f_check);
	free(workerArgs);
	deleteQueue(q);

	return 0;
}

