/* \file myconn.h
   \author Carlo Alessi
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  
*/

#ifndef MYCONN_H
#define MYCONN_H

#include "wator.h"

#define SOCKNAME "./visual.sck"  /* nome della socket */
#define MAXCONN  1 			   /* connessioni simultanee consentite al server */
#define BUFSIZE 2500 		   /* lunghezza del buffer */

typedef struct msg {
	int nrow;		/* numero righe */
	int ncol; 		/* numero colonne */
    int8_t set[BUFSIZE]; /* ogni cella dell'array contiene la codifica di 4 caratteri */
} msg_t;

/* viene effettuta la connessione al processo visualizer tramite la socket 'visual.sck'
   e viene inviata la matrice che rappresenta il pianeta.
   \param pw puntatore al pianeta
   \param termina, flag di terminazione.
   
   se termina == 1, il visualizer interpreta il messaggio come endOfStream
   se termina == 0, il flag viene ignorato e si procede con l'invio della matrice
   
   \retval 0 in caso di successo, -1 altrimenti
*/
int connectToVisualizer(wator_t* pw, int termina);

/* riceve il messaggio contenente la matrice dalla socket.
	\param c_sockfd, file descriptor del client
	\param dumpfile, puntatore a un file
	
	'dumpfile' verrà usato dalla funzione dump()
	
	\retval 0 in caso di successo, -1 altrimenti
*/
int receiveMatrix_And_Dump(int c_sockfd, FILE* dumpfile);

#endif /* CONN_H */
