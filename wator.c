/* \file wator.c
   \author Carlo Alessi
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include "wator.h"
#include "help.h"

char cell_to_char (cell_t a){
	char c;
	switch(a){
		case FISH : c = 'F'; break;
		case WATER : c = 'W'; break;
		case SHARK : c = 'S'; break;
		default: c = '?'; 
	}
	return c;
}

int char_to_cell(char c){	
	cell_t cell;
	if(c == 'S')	cell = SHARK;
	else if(c == 'F')	cell = FISH;
	else if(c == 'W')	cell = WATER;
	else cell = -1;
	return cell;
}

planet_t * new_planet (unsigned int nrow, unsigned int ncol){
	int i,j;
	planet_t* p;
	
	if(!nrow || !ncol){
		fprintf(stderr, "errore: nrow o ncol uguali a 0\n");
		return NULL;
	}	
	
	CHECK_NULL_PTR(p = malloc(sizeof(planet_t)), "malloc");
	
	p->nrow = nrow;
	p->ncol = ncol;
	
	/* alloco le matrici */
	p->w 	 = alloca_cell_t(nrow, ncol);
	p->btime = alloca_int(nrow, ncol);    
	p->dtime = alloca_int(nrow, ncol);		
	if( !p->w || !p->btime || !p->dtime){
		fprintf(stderr, "errore allocazione matrici\n");
		return NULL;
	}
	
	for(i = 0; i < nrow; i++)	/* riempio il pianeta d'acqua */
		for(j = 0; j < ncol; j++)
			p->w[i][j] = WATER;
	return p;	 
}

void free_planet (planet_t* p){
	int nrow = p->nrow;
	clean((void**)(p->btime), nrow);
	clean((void**)(p->dtime), nrow);
	clean((void**)(p->w), nrow);
	free(p);
}

int print_planet (FILE* f, planet_t* p){
	int i,j;
	
	if(!f || !p){
		fprintf(stderr, "errore: print_planet\n");
		return -1;
	} 
	
	/* ristabilisco la posizione iniziale del file all'inizio */
	if(f != stdout)
		SC_MENO1( fseek(f, 0, SEEK_SET), "fseek"); 
		
	fprintf(f, "%d\n", p->nrow); /* scrivo su file numero righe e colonne */
	fprintf(f, "%d\n", p->ncol);
	
	for(i = 0; i < p->nrow; i++){ /* scrivo su file le celle del pianeta */
		for(j = 0; j < (p->ncol)-1; j++)
			fprintf(f, "%c ", cell_to_char(p->w[i][j]));	
		fprintf(f, "%c\n", cell_to_char(p->w[i][j]));	
	}
	return 0;
}

planet_t* load_planet (FILE* f){
	unsigned int i, j, nrow, ncol;
	char c; /* carattere letto dal file */
	char b; /* per contenere un ' ' o un '\n' */
	cell_t cell;
	planet_t* p;
	
	CHECK_NULL_PTR(f, "load_planet");
	
	/* leggo il numero di righe e colonne e '\n' */
	if(fscanf(f, "%u%c", &nrow, &b) == EOF){ 
		perror("fscanf");
		return NULL;
	}
 	if(fscanf(f, "%u%c", &ncol, &b) == EOF){
 		perror("fscanf");
 		return NULL;
 	}
	
	p = new_planet(nrow, ncol); /* alloco il pianeta. è pieno di WATER per ora */
	assert(p);
	
	for(i = 0; i < nrow; i++)
		for(j = 0; j < ncol; j++){
			if(fscanf(f, "%c%c", &c, &b) == EOF){ /*leggo un char della matrice e un ' ' o '\n' */
				perror("fscanf");
				return NULL;
			}
			if((cell = char_to_cell(c)) == -1){ /* se il carattere non appartiene a {W,F,S} */
				errno = ERANGE;
				perror("char_to_cell");
				return NULL;
			}
			p->w[i][j] = cell;
		}		
				
	return p;
}

wator_t* new_wator (char* fileplan){
	int i,j;
	int num; /* numero letto dal file */
	int ns = 0, nf = 0; /* contatori  */
	int nrow, ncol;
	wator_t* pw;
	FILE* f; /* per aprire fileplan (planet1.check) */
	FILE* fconf; /* per CONFIGURATION_FILE (wator.conf) */
	char cf[3]; /* per leggere una stringa appartenente a S = {sd ,sb, fb} */
	char b; /* per leggere un ' ' o un '\n' */
	char cell; /* per la conversione cell_to_char */
	
	assert(fileplan);
	
	CHECK_NULL_PTR(pw = malloc(sizeof(wator_t)), "malloc");
	CHECK_NULL_PTR(f = fopen(fileplan, "r"),"fopen_fileplan");
	
	pw->plan = load_planet(f);
	nrow 	 = pw->plan->nrow;
	ncol 	 = pw->plan->ncol;
	
	/* conto numero pesci e squali */
	for(i = 0; i < nrow; i++)
		for(j = 0; j < ncol; j++){
			cell = cell_to_char(pw->plan->w[i][j]);
			assert(cell);
			if(cell == 'F')			nf++; /* incremento numero pesci */
		 	else if(cell == 'S')	ns++; /* incremento numero squali */
		}
		
	pw->nf = nf;
	pw->ns = ns;	

	CHECK_NULL_PTR(fconf = fopen(CONFIGURATION_FILE, "r"),"fopen_conf");
	
	for(i = 0; i < 3; i++){
		fgets(cf, sizeof(cf), fconf); /* leggo una stringa appartenente a S = {sd ,sb, fb} */
		fscanf(fconf,"%d%c",&num, &b); /* leggo numero e '\n' */
		if(strcmp(cf,"sd") == 0)		pw->sd = num;	
		else if(strcmp(cf,"sb") == 0)	pw->sb = num;
		else if(strcmp(cf,"fb") == 0)	pw->fb = num;
	}
	pw->chronon = 0;
	
	fclose(f);
	fclose(fconf);
	return pw;
}

void free_wator(wator_t* pw){
	free_planet(pw->plan);
	free(pw);
}

int shark_rule1 (wator_t* pw, int x, int y, int *k, int* l){
	coordinate_t* f;
	char c[4]; /* caratteri celle adiacenti */
	int nrow, ncol;
	int index; /* indice random per scegliere dove muoversi */
	
	f = malloc(sizeof(coordinate_t));
	CHECK_MENO_1(f, "malloc");
	
	srand(time(NULL));
	
	if(!pw){
		fprintf(stderr, "errore: shark_rule1\n");
		return -1;
	}
	
	nrow = pw->plan->nrow;
	ncol = pw->plan->ncol;
	
	f->x_up = get_coordinata(x, -1, nrow); 
	f->x_down = get_coordinata(x, +1, nrow); 
	f->y_sx = get_coordinata(y, -1, ncol);
	f->y_dx = get_coordinata(y, +1, ncol);
	
	c[0] = cell_to_char(pw->plan->w[f->x_up][y]);   /* up */
	c[1] = cell_to_char(pw->plan->w[x][f->y_dx]);   /* dx */
	c[2] = cell_to_char(pw->plan->w[f->x_down][y]); /* down */
	c[3] = cell_to_char(pw->plan->w[x][f->y_sx]);	 /* sx */
	
	if(c[0] == 'S' && c[1] == 'S' && c[2] == 'S' && c[3] == 'S'){	/* rimane fermo */
		free(f);
		return STOP;
	}
	
	/* c'è un pesce da mangiare? */
	if(c[0] == 'F' || c[1] == 'F' || c[2] == 'F' || c[3] == 'F'){
		while(1){
			index = rand() % 4;
			if(c[index] == 'F'){ /* genero casualmente un indice finché non trova un pesce */
				move_or_eat(pw, index, x, y, k, l, f);
				break;	
			} 			
		}
		free(f);
		return EAT;
	}else{							
		while(1){
			index = rand() % 4;
			if(c[index] == 'W'){ /* genero casualmente un indice finché non trova acqua */
				move_or_eat(pw, index, x, y, k, l, f);
				break;	
			} 			
		}
		free(f);
		return MOVE;
	}
}

int shark_rule2 (wator_t* pw, int x, int y, int *k, int* l){
	coordinate_t* f;
	char c[4]; /* caratteri celle adiacenti */
	int index; /* indice random per scegliere dove far nascere lo squalo */
	
	if(!pw){
		fprintf(stderr, "errore: shark_rule2\n");
		return -1;
	}
	srand(time(NULL));
	
	f = malloc(sizeof(coordinate_t));
	CHECK_MENO_1(f, "malloc");
	
	f->x_up = get_coordinata(x, -1, pw->plan->nrow); 
	f->x_down = get_coordinata(x, +1, pw->plan->nrow); 
	f->y_sx = get_coordinata(y, -1, pw->plan->ncol);
	f->y_dx = get_coordinata(y, +1, pw->plan->ncol);
	
	c[0] = cell_to_char(pw->plan->w[f->x_up][y]);   /* up */
	c[1] = cell_to_char(pw->plan->w[x][f->y_dx]);   /* dx */
	c[2] = cell_to_char(pw->plan->w[f->x_down][y]); /* down */
	c[3] = cell_to_char(pw->plan->w[x][f->y_sx]);	 /* sx */
	
	if(pw->plan->btime[x][y] < pw->sb)	
		(pw->plan->btime[x][y])++;
	else if(pw->plan->btime[x][y] == pw->sb){	/* si tenta di generare uno squalo */
		if(c[0] == 'W' || c[1] == 'W' || c[2] == 'W' || c[3] == 'W'){
			while(1){
				index = rand() % 4;
				if(c[index] == 'W'){ /* genero casualmente un indice finché non trova acqua */
					born(pw, index, x, y, k, l, f);
					break;	
				}	 			
			}
		}
		pw->plan->btime[x][y] = 0; /* azzero il contatore */
	}
	
	free(f);
	
	/* controllo morti */
	if(pw->plan->dtime[x][y] < pw->sd){
		(pw->plan->dtime[x][y])++;
		return ALIVE;
	}else if(pw->plan->dtime[x][y] == pw->sd){
		pw->plan->w[x][y] = WATER; /* lo squalo muore */
		return DEAD;
	}else	return -1;
}

int fish_rule3 (wator_t* pw, int x, int y, int *k, int* l){
	coordinate_t* f;
	char c[4]; /* caratteri celle adiacenti */
	int index; /* indice random per scegliere dove muoversi */
	
	if(!pw){
		fprintf(stderr, "errore: fish_rule3\n");
		return -1;
	}
	
	f = malloc(sizeof(coordinate_t));
	CHECK_MENO_1(f, "malloc");
	
	srand(time(NULL));
	
	f->x_up = get_coordinata(x, -1, pw->plan->nrow); 
	f->x_down = get_coordinata(x, +1, pw->plan->nrow); 
	f->y_sx = get_coordinata(y, -1, pw->plan->ncol);
	f->y_dx = get_coordinata(y, +1, pw->plan->ncol);
	
	c[0] = cell_to_char(pw->plan->w[f->x_up][y]);   /* up */
	c[1] = cell_to_char(pw->plan->w[x][f->y_dx]);   /* dx */
	c[2] = cell_to_char(pw->plan->w[f->x_down][y]); /* down */
	c[3] = cell_to_char(pw->plan->w[x][f->y_sx]);	 /* sx */
	
	if(c[0] != 'W' && c[1] != 'W' && c[2] != 'W' && c[3] != 'W'){	/* rimane fermo */
		free(f);
		return STOP;
	} else {							
		while(1){
			index = rand() % 4;
			if(c[index] == 'W'){ /* genero casualmente un indice finché non trova acqua */
				move_or_eat(pw, index, x, y, k, l, f);
				break;	
			} 			
		}
		free(f);
		return MOVE;
	}
}

int fish_rule4 (wator_t* pw, int x, int y, int *k, int* l){
	coordinate_t* pos;
	char c[4]; /* caratteri celle adiacenti */
	int index; /* indice random per scegliere dove far nascere il pesce */
	
	if(!pw){
		fprintf(stderr, "errore: fish_rule4\n");
		return -1;
	}
	
	pos = malloc(sizeof(coordinate_t));
	CHECK_MENO_1(pos, "malloc");
	
	srand(time(NULL));
	
	pos->x_up =   get_coordinata(x, -1, pw->plan->nrow); 
	pos->x_down = get_coordinata(x, +1, pw->plan->nrow); 
	pos->y_sx =	  get_coordinata(y, -1, pw->plan->ncol);
	pos->y_dx =   get_coordinata(y, +1, pw->plan->ncol);
	
	c[0] = cell_to_char(pw->plan->w[pos->x_up][y]);   /* up */
	c[1] = cell_to_char(pw->plan->w[x][pos->y_dx]);   /* dx */
	c[2] = cell_to_char(pw->plan->w[pos->x_down][y]); /* down */
	c[3] = cell_to_char(pw->plan->w[x][pos->y_sx]);	/* sx */
	
	if(pw->plan->btime[x][y] < pw->fb)	
		(pw->plan->btime[x][y])++;
	else if(pw->plan->btime[x][y] == pw->fb){	/* si tenta di generare uno pesce */
		if(c[0] == 'W' || c[1] == 'W' || c[2] == 'W' || c[3] == 'W'){
			while(1){
				index = rand() % 4;
				if(c[index] == 'W'){ 
					/* genero casualmente un indice finché non trova acqua */
					born(pw, index, x, y, k, l, pos);
					break;	
				}	 			
			}
		}
		pw->plan->btime[x][y] = 0; /* azzero il contatore */
	}
	free(pos);
	return 0;
}

int fish_count (planet_t* p){
	int i, j;
	int n = 0;
	
	if(!p){
		fprintf(stderr, "errore: fish_count\n");
		return -1;
	}
	
	for(i = 0; i < p->nrow; i++)
		for(j = 0; j < p->ncol; j++)
			if(p->w[i][j] == FISH)
				n++;
	return n;
}

int shark_count (planet_t* p){
	int i, j;
	int n = 0; /* contatore squali */
	
	if(!p){
		fprintf(stderr, "errore: shark_count\n");
		return -1;
	}
	
	for(i = 0; i < p->nrow; i++)
		for(j = 0; j < p->ncol; j++)
			if(p->w[i][j] == SHARK)
				n++;
	return n;					
}

int update_wator (wator_t * pw){
	int i, j;
	int k, l;
	int nrow, ncol;
	int ** F; /* flag */
	int ris; /* salva il risultato dei valori di ritorno delle regole */
	cell_t cell;
	
	if(!pw){
		fprintf(stderr, "errore: update_wator\n");
		return -1;
	}
	nrow = pw->plan->nrow;
	ncol = pw->plan->ncol;
	
	if(!(F = alloca_int(nrow, ncol))) { 
		fprintf(stderr, "errore: update_wator, allocazione matrice\n");
		return -1; 
	}
	
	for(i = 0; i < nrow; i++){
		k = i;
		for(j = 0; j < ncol; j++){
			l = j;
			if(F[i][j] == 1)	/* se è già stata aggiornata non fa nulla */ 
				continue;
			
			cell = pw->plan->w[i][j];
			switch(cell){
				case FISH:
					if((ris = fish_rule4(pw,i,j,&k,&l)) == -1)
						return -1;
					else /* è nato un pesce */
						F[k][l] = 1;
					
					k = i; l = j;
					if((ris = fish_rule3(pw, i, j, &k, &l)) == -1)
						return -1;
					else if(ris == MOVE) /* setta la nuova posizione del pesce a 1 */
						F[k][l] = 1;
					break;
				case SHARK:
					if((ris = shark_rule2(pw,i,j,&k,&l)) == -1)
						return -1;
					else if(ris == ALIVE){
						if(k != i || l != j) /* se ha fatto un figlio */
							F[k][l] = 1;
						
						k = i; l = j;	
						if((ris = shark_rule1(pw,i,j,&k,&l)) == -1)
							return -1;

						if(ris == MOVE || ris == EAT) /* se si è mosso oppure ha mangiato */
							F[k][l] = 1;	
					}
					break;
				default:
					break;
			} /* end switch */ 	
		} /* end for annidato */
	} /* end for */
	
	pw->nf = fish_count(pw->plan);
	pw->ns = shark_count(pw->plan);
	pw->chronon++;
	clean((void**)F, nrow);
	printf("pesci %d\nsquali %d\n",pw->nf, pw->ns);
			
	return 0;
}
