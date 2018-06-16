/* \file myconn.c
   \author Carlo Alessi
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/un.h>
#include "myconn.h"
#include "wator.h"
#include "help.h"
#include "mutex.h"

/* codifica un carattere della matrice con un intero a 8 bit. 
   visto che i possibili caratteri sono 3, saranno necessari 2 bit per identificare un carattere.
   quindi un int8_t conterrà la codifica di 4 caratteri.
   \param c carattere della matrice
   
   \retval code, codice
*/
static int8_t encode(char c){
	int8_t code;
	switch(c){
		case 'S':
			code = 0x0;
			break;
		case 'F':
			code = 0x1;
			break;
		case 'W':
			code = 0x2;
			break;  
		default:
			break;
	}
	return code;
}

/*  effettua la decodifica del intero a 8 bit per ottenere il carattere della matrice.
	\param a, intero che contiene la codifica di 4 caratteri
	\param shift, numero di bit da dover shiftare.
	
	shiftando il numero verso destra di un numero di bit stabilito dal parametro 'shift' si
	ottiene un numero i cui 2 bit meno significativi corrispondono alla codifica del carrattere
	che vogliamo decodificare.
	per estrarre i 2 bit meno significativi viene fatto l'AND bit a bit con 0x3 
	
	\retval c, carattere.
*/
static char decode(int8_t a, int shift){
	char c;
	a = (a >> shift) & 0x3; /* shift 'a' di 'shift' bit a dx, prendo i 2 bit meno significativi. */
	switch(a){
		case 0x0:
			c = 'S';
			break;
		case 0x1:
			c = 'F';
			break;
		case 0x2:
			c = 'W';
			break;
		default:
			fprintf(stderr, "errore: decode\n");
			break;	
	}
	return c;	
}

/* effettua il dump della matrice sul file 'dumpfile' oppure su standard output.
	\param dumpfile, puntatore a un file
	\param msg, puntatore al messaggio ricevuto
	
	\retval 0 in caso di successo, -1 altrimenti.
*/
static int dump(FILE* dumpfile, msg_t* msg){
	int i,j,k = 0;
	
	if(dumpfile != stdout)
		/* ristabilisco la posizione iniziale del file all'inizio */
		SC_MENO1( fseek(dumpfile, 0, SEEK_SET), "fseek");
		
	/* scrivo su file numero righe e colonne */
	fprintf(dumpfile, "%d\n", (*msg).nrow); 
	fprintf(dumpfile, "%d\n", (*msg).ncol);
	
	/* scrivo su file le celle del pianeta */
	for(i = 0; i < (*msg).nrow; i++){ 
		for(j = 0; j < (*msg).ncol - 1; j++){
			assert(k/4 < BUFSIZE);
			fprintf(dumpfile, "%c ", decode((*msg).set[k/4], 2 * (k % 4)));	
			k++;
		}
		fprintf(dumpfile, "%c\n", decode((*msg).set[k/4], 2 * (k % 4)));	
		k++;
	}
	return 0;
}

/* invia la matrice attraverso la socket. 'termina' viene usato come flag di terminazione.
   all'inizio della comunicazione viene inviato un intero per stabilire se 
   si è in fase di terminazione o no.
   \param pw, puntatore alla struttura di simulazione
   \param sockfd, file descriptor del server
   \param termina, flag terminazione
   
   \retval 0 in caso di successo, -1 altrimenti
*/
static int sendMatrix(wator_t* pw, int sockfd, int termina){
	msg_t msg;
	int i, j, k = 0;
	int t = termina;
	
	assert(termina == 0);
	memset(&msg, 0, sizeof(msg_t));
	
	SC_MENO1( write(sockfd, &t, sizeof(int)), "write2.1");
	
	LockWator();
	msg.nrow = pw->plan->nrow;
	msg.ncol = pw->plan->ncol;
	assert(msg.nrow == pw->plan->nrow);
	assert(msg.ncol == pw->plan->ncol);
	
	for(i = 0; i < msg.nrow; i++)
		for(j = 0; j < msg.ncol; j++){
			int z = k / 4;
			cell_t cell;
			char c;
			assert(z < BUFSIZE);
			
			cell = pw->plan->w[i][j];
			c = cell_to_char(cell);
			msg.set[z] = msg.set[z] | (encode(c) << 2 * (k % 4));
			k++;
		}
		
	SC_MENO1( write(sockfd, &msg, sizeof(msg_t)), "write2");
	UnlockWator();
	return 0;	
}

/*----------------------------------------------------------*/

int connectToVisualizer(wator_t* pw, int termina){
	int sockfd, len, res;
	struct sockaddr_un address;
	
	/* crea la socket */
	SC_MENO1((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)), "socket");
	/* da un nome alla socket in accordo con il server */
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, SOCKNAME);
	len = sizeof(address);
	/* connessione al processo server */
	while( connect(sockfd, (struct sockaddr*)&address, len) == -1){
		if(errno != ENOENT && errno != ECONNREFUSED){
			perror("connect");
			return -1;
		}
		/* la socket ancora non esiste, riprovo a connettermi */
	}
	
	if(termina == 1){
		/* invia messaggio di terminazione */
		int t = termina;
		SC_MENO1( write(sockfd, &t, sizeof(int)), "write1");
		SC_MENO1( close(sockfd), "close");
	} else {
		/* manda matrice al processo visualizer */
		assert(termina == 0);
 		res = sendMatrix(pw, sockfd, 0); /* flag non terminazione */
 		SC_MENO1( close(sockfd), "close");
 		if(res == -1){
 			fprintf(stderr, "errore: invio matrice fallito\n");
 			return -1;
 		}
	}
	return 0;
}

int receiveMatrix_And_Dump(int c_sockfd, FILE* dumpfile){
	msg_t msg;
	int res;

	memset(&msg, 0, sizeof(msg_t));
	
	/* legge il messaggio */
	SC_MENO1( read(c_sockfd, &msg, sizeof(msg_t)), "read");
	
	/* stampo la matrice */			
	if(dumpfile)
		res = dump(dumpfile, &msg);
	else
		res = dump(stdout, &msg);		
	
	if(res != 0){
		fprintf(stderr, "errore: dump della matrice non riuscito\n");
		return -1;
	}
	
	return 0;
}

