#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <omp.h>

// Prototipos de funciones
double** reservar_matriz(int n);
void liberar_matriz(double** matriz, int n);
void llenar_matriz_aleatoria(double** matriz, int n);
void imprimir_matriz(double** matriz, int n);
double** multiplicar_matrices_openmp(double** A, double** B, int n, int num_hilos);
void mostrar_ayuda();

// Función para reservar memoria para una matriz cuadrada de tamaño n x n
double** reservar_matriz(int n) {
    double** matriz = (double**)malloc(n * sizeof(double*));
    if (matriz == NULL) {
        fprintf(stderr, "Error en la asignación de memoria para la matriz\n");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < n; i++) {
        matriz[i] = (double*)malloc(n * sizeof(double));
        if (matriz[i] == NULL) {
            fprintf(stderr, "Error en la asignación de memoria para la fila %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    
    return matriz;
}

// Función para liberar la memoria de una matriz
void liberar_matriz(double** matriz, int n) {
    for (int i = 0; i < n; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

// Función para llenar una matriz con valores aleatorios
void llenar_matriz_aleatoria(double** matriz, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matriz[i][j] = (double)(rand() % 10);
        }
    }
}

// Función para imprimir una matriz
void imprimir_matriz(double** matriz, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%lf ", matriz[i][j]);
        }
        printf("\n");
    }
}

// Función para multiplicar dos matrices usando OpenMP
double** multiplicar_matrices_openmp(double** A, double** B, int n, int num_hilos) {
    double** C = reservar_matriz(n);
    
    // Establecer el número de hilos para OpenMP
    omp_set_num_threads(num_hilos);
    
    // Multiplicación de matrices con paralelización de OpenMP
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = 0.0;
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    
    return C;
}

// Función para mostrar ayuda
void mostrar_ayuda() {
    printf("Uso: ./programa [-t tamaño] [-h hilos] [-p]\n");
    printf("Opciones:\n");
    printf("  -t, --tamano    Tamaño de las matrices cuadradas (por defecto: 3)\n");
    printf("  -h, --hilos     Número de hilos a utilizar con OpenMP (por defecto: 4)\n");
    printf("  -p, --imprimir  Imprimir las matrices (opcional)\n");
    printf("  -a, --ayuda     Mostrar esta ayuda\n");
}

int main(int argc, char *argv[]) {
    // Valores por defecto
    int n = 3;           // Tamaño de la matriz
    int num_hilos = 4;   // Número de hilos
    int imprimir = 0;    // No imprimir matrices por defecto
    
    // Definir las opciones para getopt_long
    static struct option opciones_largas[] = {
        {"tamano", required_argument, 0, 't'},
        {"hilos", required_argument, 0, 'h'},
        {"imprimir", no_argument, 0, 'p'},
        {"ayuda", no_argument, 0, 'a'},
        {0, 0, 0, 0}
    };
    
    int opcion;
    int indice_opcion = 0;
    
    // Procesar los argumentos de la línea de comandos
    while ((opcion = getopt_long(argc, argv, "t:h:pa", opciones_largas, &indice_opcion)) != -1) {
        switch (opcion) {
            case 't':
                n = atoi(optarg);
                if (n <= 0) {
                    fprintf(stderr, "El tamaño de la matriz debe ser positivo\n");
                    return EXIT_FAILURE;
                }
                break;
            case 'h':
                num_hilos = atoi(optarg);
                if (num_hilos <= 0) {
                    fprintf(stderr, "El número de hilos debe ser positivo\n");
                    return EXIT_FAILURE;
                }
                break;
            case 'p':
                imprimir = 1;
                break;
            case 'a':
                mostrar_ayuda();
                return EXIT_SUCCESS;
            case '?':
                // getopt_long ya imprime un mensaje de error
                mostrar_ayuda();
                return EXIT_FAILURE;
            default:
                return EXIT_FAILURE;
        }
    }
    
    // Inicializar el generador de números aleatorios
    srand(time(NULL));
    
    // Reservar memoria para las matrices
    double** A = reservar_matriz(n);
    double** B = reservar_matriz(n);
    
    // Llenar las matrices con valores aleatorios
    llenar_matriz_aleatoria(A, n);
    llenar_matriz_aleatoria(B, n);
    
    // Medir tiempo de ejecución con clock() como en el ejemplo proporcionado
    clock_t inicio_clock = clock();
    
    // Medición de tiempo más precisa usando OpenMP
    double inicio_omp = omp_get_wtime();
    
    // Multiplicar las matrices usando OpenMP
    double** C = multiplicar_matrices_openmp(A, B, n, num_hilos);
    
    // Finalizar medición del tiempo
    double fin_omp = omp_get_wtime();
    clock_t fin_clock = clock();
    
    double tiempo_clock = (double)(fin_clock - inicio_clock) / CLOCKS_PER_SEC;
    double tiempo_omp = fin_omp - inicio_omp;
    
    // Imprimir las matrices si se solicitó
    if (imprimir) {
        printf("\nMatriz A:\n");
        imprimir_matriz(A, n);
        
        printf("\nMatriz B:\n");
        imprimir_matriz(B, n);
        
        printf("\nMatriz Resultado (C = A * B):\n");
        imprimir_matriz(C, n);
    }
    
    // Imprimir estadísticas
    // printf("\nEstadísticas:\n");
    // printf("- Tamaño de la matriz: %d x %d\n", n, n);
    // printf("- Número de hilos utilizados: %d\n", num_hilos);
    // printf("- Tiempo de ejecución (clock): %.6f segundos\n", tiempo_clock);
    printf("- Tiempo de ejecución (OpenMP): %.6f segundos\n", tiempo_omp);
    
    // Liberar memoria
    liberar_matriz(A, n);
    liberar_matriz(B, n);
    liberar_matriz(C, n);
    
    return EXIT_SUCCESS;
}