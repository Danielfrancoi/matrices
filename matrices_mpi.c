/*
 * matrices_mpi.c
 *
 * Multiplicación de matrices cuadradas con paralelización usando MPI.
 * Basado en el archivo matrices_secuencial.c, adaptado para distribución de trabajo
 * entre procesos MPI. Cada proceso calcula un conjunto de filas de la matriz resultado.
 *
 * Uso:
 *   mpicc matrices_mpi.c -o matrices_mpi
 *   mpirun -np <num_procesos> ./matrices_mpi -n <dimension_matriz>
 *
 * Ejemplo:
 *   mpirun -np 4 ./matrices_mpi -n 1000
 *
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>

/* Reserva memoria contigua para una matriz de tamaño N x N */
double* reservar_matriz(int N) {
    return (double*)malloc(N * N * sizeof(double));
}

/* Función para llenar una matriz N x N con valores aleatorios */
void llenar_matriz(double* M, int N) {
    for (int i = 0; i < N * N; i++) {
        M[i] = (double)(rand() % 10);
    }
}

/* Función para imprimir una matriz N x N */
void imprimir_matriz(double* M, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf("%6.2f ", M[i * N + j]);
        }
        printf("\n");
    }
}

/* Obtiene el número de filas asignadas al proceso rank, dado N y size.
 * Se reparte la división entera, y los procesos con rank < (N % size) reciben una fila extra.
 */
int filas_por_proceso(int rank, int size, int N) {
    int base = N / size;
    int resto = N % size;
    if (rank < resto) {
        return base + 1;
    } else {
        return base;
    }
}

/* Calcula los desplazamientos (en número de elementos) para Scatterv/Gatherv */
void calcular_desplazamientos(int* counts, int* displs, int size, int N) {
    /* counts[i] = número de elementos (floats) que recibe el proceso i */
    /* displs[i] = desplazamiento (offset) en el arreglo global (en elementos) */
    int desplazamiento = 0;
    for (int i = 0; i < size; i++) {
        int filas = filas_por_proceso(i, size, N);
        counts[i] = filas * N;          // cada fila tiene N elementos
        displs[i] = desplazamiento;
        desplazamiento += counts[i];
    }
}

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>

int main(int argc, char* argv[]) {
    int rank, size;
    int N = 3;  // dimensión por defecto (3x3), si no se especifica -n
    int opt;

    /* Inicializar MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Procesar opciones de línea de comandos */
    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
            case 'n':
                N = atoi(optarg);
                break;
            default:
                if (rank == 0) {
                    fprintf(stderr, "Uso: %s -n <dimension_matriz>\n", argv[0]);
                }
                MPI_Finalize();
                exit(EXIT_FAILURE);
        }
    }

    /* Verificar que N > 0 */
    if (N <= 0) {
        if (rank == 0) {
            fprintf(stderr, "La dimensión de la matriz debe ser un entero positivo.\n");
        }
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    /* Verificar que N >= número de procesos, de lo contrario algunos procesos no tendrían filas */
    if (N < size) {
        if (rank == 0) {
            fprintf(stderr,
                    "Advertencia: la dimensión de la matriz (%d) es menor que el número de procesos (%d).\n"
                    "Algunos procesos no recibirán filas para procesar.\n", N, size);
        }
    }

    /* Reservar punteros para las matrices A y B en el root */
    double* A = NULL;
    double* B = NULL;
    double* C = NULL;  // matriz resultado completa, sólo en root

    /* Cada proceso necesita espacio para B completo y su porción de A y C */
    double* B_local = (double*)malloc(N * N * sizeof(double));  // se llenará vía MPI_Bcast
    if (B_local == NULL) {
        fprintf(stderr, "Error al asignar memoria para B_local en proceso %d\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    /* Calcular cuántas filas maneja cada proceso */
    int filas_local = filas_por_proceso(rank, size, N);

    /* Arreglos temporales para recuentos y desplazamientos */
    int* sendcounts = NULL;
    int* displs = NULL;
    int* recvcounts = NULL;
    int* recvdispls = NULL;

    /* Sólo el root inicializa sendcounts y displs para distribuir A */
    if (rank == 0) {
        A = reservar_matriz(N);
        B = reservar_matriz(N);
        C = reservar_matriz(N);  // se usará al final para recoger resultados

        if (A == NULL || B == NULL || C == NULL) {
            fprintf(stderr, "Error al asignar memoria en root\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        /* Llenar A y B con valores aleatorios en root */
        srand(time(NULL));
        llenar_matriz(A, N);
        llenar_matriz(B, N);

        /* Opcional: imprimir las matrices A y B
        printf("Matriz A (root):\n");
        imprimir_matriz(A, N);
        printf("Matriz B (root):\n");
        imprimir_matriz(B, N);
        */

        /* Preparar vectores para Scatterv y Gatherv */
        sendcounts = (int*)malloc(size * sizeof(int));
        displs = (int*)malloc(size * sizeof(int));
        recvcounts = (int*)malloc(size * sizeof(int));
        recvdispls = (int*)malloc(size * sizeof(int));

        if (sendcounts == NULL || displs == NULL || recvcounts == NULL || recvdispls == NULL) {
            fprintf(stderr, "Error al asignar memoria para sendcounts/displs en root\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        /* Calcular cómo repartir las filas de A entre procesos */
        calcular_desplazamientos(sendcounts, displs, size, N);

        /* Para recoger los resultados de C, la distribución es igual a la de A */
        /* recvcounts[i] = filas_por_proceso(i)*N, y recvdispls = mismo desplazamiento */
        for (int i = 0; i < size; i++) {
            recvcounts[i] = sendcounts[i];   // misma lógica, mismos elementos
            recvdispls[i] = displs[i];
        }
    }

    /* Broadcast de la dimensión N a todos los procesos */
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /* Cada proceso reserva espacio para su porción de A y C */
    double* A_local = (double*)malloc(filas_local * N * sizeof(double));
    double* C_local = (double*)malloc(filas_local * N * sizeof(double));
    if (A_local == NULL || C_local == NULL) {
        fprintf(stderr, "Error al asignar memoria para A_local o C_local en proceso %d\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    /* Root envía la matriz B completa a todos los procesos */
    /* Primero, root copia su B a B_local, otros procesos B_local no inicializado */
    if (rank == 0) {
        for (int i = 0; i < N * N; i++) {
            B_local[i] = B[i];
        }
    }
    MPI_Bcast(B_local, N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    /* Scatterv para distribuir las filas de A entre procesos */
    MPI_Scatterv(
        A,                /* buffer origen en root */
        sendcounts,       /* número de elementos enviados a cada proceso */
        displs,           /* desplazamientos en A (en elementos) */
        MPI_DOUBLE,       /* tipo de datos */
        A_local,          /* buffer destino local */
        filas_local * N,  /* número de elementos que recibe este proceso */
        MPI_DOUBLE,       /* tipo de datos */
        0,                /* root */
        MPI_COMM_WORLD
    );

    /* Sincronizar antes de comenzar la multiplicación y medir tiempo */
    MPI_Barrier(MPI_COMM_WORLD);
    double t_inicio = MPI_Wtime();

    /* Inicializar C_local a cero */
    for (int i = 0; i < filas_local * N; i++) {
        C_local[i] = 0.0;
    }

    /* Multiplicación parcial: cada proceso calcula sus filas asignadas */
    /* A_local tiene filas_local filas, cada una con N columnas */
    /* B_local es N x N */
    for (int i = 0; i < filas_local; i++) {
        for (int j = 0; j < N; j++) {
            double sum = 0.0;
            for (int k = 0; k < N; k++) {
                sum += A_local[i * N + k] * B_local[k * N + j];
            }
            C_local[i * N + j] = sum;
        }
    }

    /* Sincronizar para finalizar el tiempo de cálculo */
    MPI_Barrier(MPI_COMM_WORLD);
    double t_final = MPI_Wtime();
    double tiempo_local = t_final - t_inicio;

    /* Root puede calcular el tiempo máximo sobre todos los procesos */
    double tiempo_max;
    MPI_Reduce(&tiempo_local, &tiempo_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    /* Reunir todas las porciones de C_local en C (en root) */
    MPI_Gatherv(
        C_local,            /* buffer origen local */
        filas_local * N,    /* número de elementos enviados por este proceso */
        MPI_DOUBLE,         /* tipo de datos */
        C,                  /* buffer destino en root */
        recvcounts,         /* número de elementos que recibirá cada proceso */
        recvdispls,         /* desplazamientos en C (en elementos) */
        MPI_DOUBLE,         /* tipo de datos */
        0,                  /* root */
        MPI_COMM_WORLD
    );

    /* Solo el root muestra el tiempo total de ejecución */
    if (rank == 0) {
        printf("Multiplicación de matrices cuadradas de dimensión %d realizada con %d procesos.\n", N, size);
        printf("Tiempo de ejecución (tiempo máximo de un proceso): %f segundos\n", tiempo_max);

        /* Opcional: imprimir la matriz resultado C
        printf("Matriz Resultado C:\n");
        imprimir_matriz(C, N);
        */

        /* Liberar memoria en root */
        free(A);
        free(B);
        free(C);
        free(sendcounts);
        free(displs);
        free(recvcounts);
        free(recvdispls);
    }

    /* Liberar memoria en cada proceso */
    free(A_local);
    free(B_local);
    free(C_local);

    /* Finalizar MPI */
    MPI_Finalize();
    return 0;
}
