#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

//#define VERBOSE

int calcMax(int** A, int i, int j, int DIM) {
	int max = 0;
	for (int r = i - 1; r <= i + 1; r++) {
		if (r >= 0 && r < DIM) {
			for (int c = j - 1; c <= j + 1; c++) {
				if (c >= 0 && c < DIM) {
					if (A[r][c] > max) {
						max = A[r][c];
					}
				}
			}
		}
	}
	return max;
}

void freeArray(int **a, int m) {
	int i;
	for (i = 0; i < m; ++i) {
		free(a[i]);
	}
	free(a);
}

int main(int argc, char* argv[]) {
	double start, end;
	int DIM, size, my_rank;

	if (argc != 3) {
		printf("\nSintassi sbagliata -- %s DIM nthreads", argv[0]);
		exit(1);
	}

	DIM = atoi(argv[1]);
	size = atoi(argv[2]);
	if (size > DIM * DIM) {
		size = DIM * DIM;
	}

	int** A = (int**)malloc(DIM * sizeof(int*));
	int** B = (int**)malloc(DIM * sizeof(int*));
	for (int i = 0; i < DIM; i++) {
		A[i] = (int*)malloc(DIM * sizeof(int));
		B[i] = (int*)malloc(DIM * sizeof(int));
	}

	// Inizializzazione matrice A
	int i, j;
	srand((unsigned int)time(NULL));
	for (i = 0; i < DIM; i++) {
		for (j = 0; j < DIM; j++) {
			A[i][j] = rand() % 50;
		}
	}

	#ifdef VERBOSE
		printf("\n[Master] matrice A:\n");
		for (i = 0; i < DIM; i++) {
			for (j = 0; j < DIM; j++) {
				printf(" %d ", A[i][j]);
			}
			printf("\n");
		}
	#endif

	// Via al cronometro
	start = omp_get_wtime();

	// Al massimo un thread a cella
	#pragma omp parallel num_threads(size) shared(A, B) private(i, j, my_rank)
	{
		my_rank = omp_get_thread_num();
		#pragma omp for schedule(static, DIM/(int)size)
			for (i = 0; i < DIM; i++) {
				for (j = 0; j < DIM; j++) {
					//printf("\n[Thread %d]\tposizione %d\t(i=%d,\tj=%d)\n", my_rank, i*DIM+j, i, j);
					B[i][j] = calcMax(A, i, j, DIM);
				}
			}
	}

	// Stop al cronometro
	end = omp_get_wtime();
	#ifdef VERBOSE
		printf("\n[Master] Risultato B:\n");
		for (i = 0; i < DIM; i++) {
			for (j = 0; j < DIM; j++) {
				printf(" %d ", B[i][j]);
			}
			printf("\n");
		}
	#endif
	printf("%f\n", end - start);

	freeArray(A, DIM);
	freeArray(B, DIM);

	return EXIT_SUCCESS;
}
