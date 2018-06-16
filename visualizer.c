/* \file visualizer.c
   \author Carlo Alessi
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "myconn.h"
#include "help.h"
#include "mutex.h"

int main(int argc, char* argv[]){
	int s_sockfd, c_sockfd;
	FILE* dumpfile = NULL;
	struct sockaddr_un address;
	int len, res;
	int t; /* flag per la terminazione */
	
	if(argv[1]){
		dumpfile = fopen(argv[1], "w+");
		CHECK_NULL(dumpfile, "fopen");
	}	
	
	memset(&address, 0, sizeof(struct sockaddr_un));
	unlink(SOCKNAME);
	/* creo una nuova socket per il server */
	SYS_CALL(s_sockfd = socket(AF_UNIX, SOCK_STREAM, 0), "socket");
	/* do un nome al socket */
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, SOCKNAME);
	len = sizeof(address);
	SYS_CALL(bind(s_sockfd, (struct sockaddr*)&address, len), "bind");
	/* mette la socket in ascolto */
	SYS_CALL(listen(s_sockfd, MAXCONN), "listen");
	
	while(1)
	{
		/* accetta una connessione */
		c_sockfd = accept(s_sockfd, NULL, 0);
		SYS_CALL(c_sockfd, "accept");
		
		SYS_CALL( read(c_sockfd, &t, sizeof(int)), "read");
		if(t == 1){
			/* è stato inviato il messaggio di terminazione */
			SYS_CALL(close(c_sockfd), "close");
		 	break;
		 } 
		
		/* riceve la matrice ed effettua il dump sul file oppure stampa a schermo */
		res = receiveMatrix_And_Dump(c_sockfd, dumpfile);
		if(res == -1){
			fprintf(stderr, "ricezione matrice fallita\n");
			SYS_CALL(close(c_sockfd), "close");
			return EXIT_FAILURE;
		}
		
		/* chiude la connessione */
		SYS_CALL(close(c_sockfd), "close");
	}
	
	if(dumpfile) /* solo se è stato specificato il dumpfile si chiude il file */
		fclose(dumpfile);
	unlink(SOCKNAME);
	return 0;
}
