#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

//#define VERBOSE

void calc(int *max, int* recv_buf, int j, int k, int DIM) {
	if (j >= 0 && k < DIM*DIM) { // controllo sia dentro i limiti
		if (recv_buf[j] > *max) {
			*max = recv_buf[j];
		}
	}
}

int calcMax(int* recv_buf, int i, int index_A, int DIM, int rank) {
	int max = 0;
	
	if ((index_A+1) % DIM != 0) { // se i NON si trova sul lato DESTRO di A
		calc(&max, recv_buf, i-(DIM-1), index_A-(DIM-1), DIM);	 // sopra destra
		calc(&max, recv_buf, i+1, index_A+1, DIM);		   		 // destra
		calc(&max, recv_buf, i+(DIM+1), index_A+(DIM+1), DIM);	 // sotto destra
	} 
	
	if (index_A % DIM != 0) { // se i NON si trova sul lato SINISTRO di A
		calc(&max, recv_buf, i-(DIM+1), index_A-(DIM+1), DIM);	 // sopra sinistra
		calc(&max, recv_buf, i-1, index_A-1, DIM);		   		 // sinistra
		calc(&max, recv_buf, i+(DIM-1), index_A+(DIM-1), DIM);	 // sotto sinistra
	}
	
	calc(&max, recv_buf, i-(DIM), index_A-(DIM), DIM);			// sopra
	calc(&max, recv_buf, i, index_A, DIM);						// centro
	calc(&max, recv_buf, i+(DIM), index_A+(DIM), DIM);			// sotto

	return max;
}

int main(int argc, char *argv[]) {
	double tempo_inizio, tempo_fine;
	int i, j;
	unsigned long long int DIM;			// dimensione della matrice A
	int* A;				 	// matrice A
	int rank;				// rank del processo
	int size;			   	// numero totale di processi
	
	int *sendcounts;					// array: ciascun elemento e' il numero di elementi da mandare ad ogni processo
	int *displs;						// array: ciascun elemento i e' la posizione iniziale (rispetto ad A) del sendcounts[i]
	int *displs_expanded_noLimit;		// displs con l'aggiunta di DIM+1 in testa e in coda
	int *sendcounts_expanded;			// sendcounts, con l'aggiunta di DIM+1 in testa e in coda, tolti i valori in posizione <0 e >DIM
	int *displs_expanded;				// displacement di sendcounts_expanded
	
	int resto;			  // elementi rimanenti da distribuire tra i processi
	int somma = 0;		  // usato per calcolare i displacements
	int *recv_buf;		  // buffer di ricezione della scatterv
	int dim_recv_buf = 0; // dimensione di recv_buf
	int *out_buf;		  // buffer di invio nella gatherv
	int dim_out_buf = 0;  // dimensione di out_buf

	if (argc != 2) {
		printf("\nSintassi sbagliata -- %s DIM", argv[0]);
		exit(1);
	}

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	DIM = atoi(argv[1]);
	#ifdef VERBOSE
		if (rank == 0) {
			printf("\n[Rank 0] DIM: %d, Number processes: %d", size, DIM);
		}
	#endif

	// Inizializzazione matrice A
	if (rank == 0) {
		A = (int*)malloc(DIM * DIM * sizeof(int));

		srand((unsigned int) time(NULL));
		for (i = 0; i < DIM*DIM; i++) {
			A[i] = rand() % 50;
		}
		#ifdef VERBOSE
			printf("\n[Rank 0] matrice A:\n");
			for (i = 0; i < DIM*DIM; i++) {
				printf(" %d ", A[i]);
				if((i+1) % DIM == 0) {
					printf("\n");
				}
			}
		#endif

		// Via al cronometro
		tempo_inizio = MPI_Wtime();
	}

	resto = (DIM*DIM)%size;
	sendcounts = (int*)malloc(size*sizeof(int));
	displs = (int*)malloc(size*sizeof(int));
	sendcounts_expanded = (int*)malloc(size*sizeof(int));
	displs_expanded = (int*)malloc(size*sizeof(int));
	displs_expanded_noLimit = (int*)malloc(size*sizeof(int));

	// Ogni processo calcola sendcounts e displs
	for (int i = 0; i < size; i++) {
		sendcounts[i] = DIM*DIM/size;
		if (resto > 0) {
			sendcounts[i]++;
			resto--;
		}

		displs[i] = somma;
		somma += sendcounts[i];

		// Calcolo della dimensione di out_buf in modo che sia piu' piccola possibile
		if(sendcounts[i] > dim_out_buf) {
			dim_out_buf = sendcounts[i];
		}
	}

	out_buf = (int*)malloc(dim_out_buf * sizeof(int));
	dim_recv_buf = dim_out_buf + (DIM+1)*2; // si aggiungono DIM elementi in testa e in coda al segmento designato al processo
	recv_buf = (int*)malloc(dim_recv_buf * sizeof(int));

	// Ogni processo calcola gli altri array di supporto
	for (int i = 0; i < size; i++) {
		displs_expanded[i] = displs[i] - (DIM+1);
		sendcounts_expanded[i] = sendcounts[i] + (DIM+1)*2;

		displs_expanded_noLimit[i] = displs_expanded[i];

		// caso segmento in testa della matrice A
		if (displs_expanded[i] < 0) {
			sendcounts_expanded[i] += displs_expanded[i];
			displs_expanded[i] = 0;
		}
		// caso segmento in coda della matrice A
		if (displs_expanded[i] + sendcounts_expanded[i] > DIM*DIM) {
			sendcounts_expanded[i] -= (displs_expanded[i] + sendcounts_expanded[i]) - DIM*DIM;
		}
	}

	// Stampa sendcounts e displs per ogni processo
	#ifdef VERBOSE
		if (rank == 0) {
			for (int i = 0; i < size; i++) {
				printf("sendcounts[%d] = %d\t\tdispls[%d] = %d\n", i, sendcounts[i], i, displs[i]);
				printf("sendcounts_exp[%d] = %d\t\tdispls_exp[%d] = %d\n", i, sendcounts_expanded[i], i, displs_expanded[i]);
				printf("displs_exp_noLimit[%d] = %d\n", i, displs_expanded_noLimit[i]);
			}
		}
	#endif

	// Scatter A ai processi in base a sendcounts_expanded e displs_expanded
	MPI_Scatterv(A, sendcounts_expanded, displs_expanded, MPI_INT, recv_buf, dim_recv_buf, MPI_INT, 0, MPI_COMM_WORLD);

	// Per ogni elemento di questo processo, calcolo il massimo nell'intorno
	#ifdef VERBOSE 
		printf("[Rank %d]: ", rank); 
	#endif
	for (int i = 0; i < sendcounts[rank]; i++) {
		// Se ci sono meno elementi del previsto e se il segmento e' all'inizio della matrice A
		if (sendcounts_expanded[rank] < sendcounts[rank] + (DIM+1)*2 && displs_expanded[rank] == 0) {
			out_buf[i] = calcMax(recv_buf, displs_expanded_noLimit[rank] + DIM+1 +i, displs[rank]+i, DIM, rank);
			#ifdef VERBOSE 
				printf("%d\t", recv_buf[displs_expanded_noLimit[rank] + DIM+1 +i]);
			#endif
		}
		// tutti gli altri casi
		else {
			out_buf[i] = calcMax(recv_buf, DIM+1 +i, displs[rank]+i, DIM, rank);
			#ifdef VERBOSE 
				printf("%d\t", recv_buf[DIM+1 +i]);
			#endif
		}
	}
	#ifdef VERBOSE 
		printf("\n");
	#endif

	// Raccolgo i segmenti da tutti i processi
	MPI_Gatherv(out_buf, sendcounts[rank], MPI_INT, A, sendcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);

	// Stampa del risultato
	if (rank == 0) {
		// Stop al cronometro
		tempo_fine = MPI_Wtime();
		#ifdef VERBOSE
			printf("\n[Rank 0] matrice risultato:\n");
			for (i = 0; i < DIM*DIM; i++) {
				printf(" %d ", A[i]);
				if((i+1) % DIM == 0) {
					printf("\n");
				}
			}
		#endif
		printf("%f\n", tempo_fine - tempo_inizio);
	}

	MPI_Finalize();

	if (rank == 0) free(A);
	free(sendcounts);
	free(displs);
	free(sendcounts_expanded);
	free(displs_expanded);
	free(displs_expanded_noLimit);
	free(recv_buf);
	free(out_buf);

	return EXIT_SUCCESS;
}