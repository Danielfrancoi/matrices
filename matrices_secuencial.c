#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>

// Función para reservar memoria para una matriz de tamaño filas x columnas
double** reservar_matriz(int filas, int columnas) {
    double** matriz = (double**)malloc(filas * sizeof(double*));
    for (int i = 0; i < filas; i++) {
        matriz[i] = (double*)malloc(columnas * sizeof(double));
    }
    return matriz;
}

// Función para liberar la memoria de una matriz
void liberar_matriz(double** matriz, int filas) {
    for (int i = 0; i < filas; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

// Función para llenar una matriz con valores aleatorios
void llenar_matriz(double** matriz, int filas, int columnas) {
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            matriz[i][j] = (double)(rand() % 10);
        }
    }
}

// Función para imprimir una matriz
void imprimir_matriz(double** matriz, int filas, int columnas) {
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            printf("%lf ", matriz[i][j]);
        }
        printf("\n");
    }
}

// Función para multiplicar dos matrices
double** multiplicar_matrices(double** A, double** B, int filasA, int columnasA, int columnasB) {
    
    double** C = reservar_matriz(filasA, columnasB);
    for (int i = 0; i < filasA; i++) {
        for (int j = 0; j < columnasB; j++) {
            C[i][j] = 0;
            for (int k = 0; k < columnasA; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    return C;
}

int main(int argc, char *argv[]) {
    int filasA = 3, columnasA = 3, filasB = 3, columnasB = 3;
    int opt;

    // Configurar opciones de línea de comandos
    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
            case 't': {
                filasA = atoi(optarg);
                columnasA = atoi(optarg);
                filasB = atoi(optarg);
                columnasB = atoi(optarg);
                break;
            }
            default:
                fprintf(stderr, "Uso: %s -r filasA -c columnasA -p filasB -q columnasB\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Verificar compatibilidad de dimensiones para la multiplicación
    if (columnasA != filasB) {
        printf("No se pueden multiplicar las matrices, las columnas de A deben coincidir con las filas de B.\n");
        return 1;
    }

    srand(time(NULL)); // Inicializar generador de números aleatorios

    // Reservar memoria para las matrices
    double** A = reservar_matriz(filasA, columnasA);
    double** B = reservar_matriz(filasB, columnasB);

    // Llenar las matrices con valores aleatorios
    llenar_matriz(A, filasA, columnasA);
    llenar_matriz(B, filasB, columnasB);

    // Multiplicar las matrices
    clock_t inicio = clock(); // Iniciar medición del tiempo
    double** C = multiplicar_matrices(A, B, filasA, columnasA, columnasB);
    clock_t fin = clock(); // Finalizar medición del tiempo
    double tiempo_ejecucion = (double)(fin - inicio) / CLOCKS_PER_SEC;
    printf("Tiempo de ejecución de la multiplicación: %f segundos\n", tiempo_ejecucion);
    
    // // Mostrar resultado
    // printf("Matriz A:\n");
    // imprimir_matriz(A, filasA, columnasA);
    // printf("Matriz B:\n");
    // imprimir_matriz(B, filasB, columnasB);
    // printf("Matriz Resultado (AxB):\n");
    // imprimir_matriz(C, filasA, columnasB);

    // Liberar memoria
    liberar_matriz(A, filasA);
    liberar_matriz(B, filasB);
    liberar_matriz(C, filasA);

    return 0;
}
