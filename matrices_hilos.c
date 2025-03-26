#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

// Estructura para pasar datos a los hilos
typedef struct
{
    int **A;
    int **B;
    int **C;
    int fila_inicio;
    int fila_fin;
    int n;
} DatosHilo;

// Prototipos de funciones
int **crear_matriz(int n);
void llenar_matriz_aleatoria(int **matriz, int n);
void imprimir_matriz(int **matriz, int n);
void liberar_matriz(int **matriz, int n);
void *multiplicar_matrices_hilo(void *arg);
void multiplicar_matrices(int **A, int **B, int **C, int n, int num_hilos);

// Función para crear una matriz cuadrada de tamaño n x n
int **crear_matriz(int n)
{
    int **matriz = (int **)malloc(n * sizeof(int *));
    if (matriz == NULL)
    {
        fprintf(stderr, "Error en la asignación de memoria\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++)
    {
        matriz[i] = (int *)malloc(n * sizeof(int));
        if (matriz[i] == NULL)
        {
            fprintf(stderr, "Error en la asignación de memoria\n");
            exit(EXIT_FAILURE);
        }
    }

    return matriz;
}

// Función para llenar una matriz con números aleatorios
void llenar_matriz_aleatoria(int **matriz, int n)
{
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            matriz[i][j] = rand() % 10; // Números aleatorios entre 0 y 9
        }
    }
}

// Función para imprimir una matriz
void imprimir_matriz(int **matriz, int n)
{
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            printf("%d ", matriz[i][j]);
        }
        printf("\n");
    }
}

// Función para liberar la memoria de una matriz
void liberar_matriz(int **matriz, int n)
{
    for (int i = 0; i < n; i++)
    {
        free(matriz[i]);
    }
    free(matriz);
}

// Función que ejecutará cada hilo para multiplicar una porción de las matrices
void *multiplicar_matrices_hilo(void *arg)
{
    DatosHilo *datos = (DatosHilo *)arg;
    int **A = datos->A;
    int **B = datos->B;
    int **C = datos->C;
    int fila_inicio = datos->fila_inicio;
    int fila_fin = datos->fila_fin;
    int n = datos->n;

    for (int i = fila_inicio; i < fila_fin; i++)
    {
        for (int j = 0; j < n; j++)
        {
            C[i][j] = 0;
            for (int k = 0; k < n; k++)
            {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    pthread_exit(NULL);
}

// Función para multiplicar matrices utilizando hilos
void multiplicar_matrices(int **A, int **B, int **C, int n, int num_hilos)
{
    pthread_t *hilos = (pthread_t *)malloc(num_hilos * sizeof(pthread_t));
    DatosHilo *datos_hilos = (DatosHilo *)malloc(num_hilos * sizeof(DatosHilo));

    if (hilos == NULL || datos_hilos == NULL)
    {
        fprintf(stderr, "Error en la asignación de memoria para hilos\n");
        exit(EXIT_FAILURE);
    }

    int filas_por_hilo = n / num_hilos;
    int filas_restantes = n % num_hilos;
    int fila_actual = 0;

    // Crear hilos para multiplicar las matrices
    for (int i = 0; i < num_hilos; i++)
    {
        datos_hilos[i].A = A;
        datos_hilos[i].B = B;
        datos_hilos[i].C = C;
        datos_hilos[i].fila_inicio = fila_actual;
        datos_hilos[i].n = n;

        // Distribuir filas restantes equitativamente
        int filas_este_hilo = filas_por_hilo;
        if (filas_restantes > 0)
        {
            filas_este_hilo++;
            filas_restantes--;
        }

        fila_actual += filas_este_hilo;
        datos_hilos[i].fila_fin = fila_actual;

        if (pthread_create(&hilos[i], NULL, multiplicar_matrices_hilo, (void *)&datos_hilos[i]) != 0)
        {
            fprintf(stderr, "Error al crear el hilo %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < num_hilos; i++)
    {
        if (pthread_join(hilos[i], NULL) != 0)
        {
            fprintf(stderr, "Error al esperar por el hilo %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    free(hilos);
    free(datos_hilos);
}

void mostrar_ayuda()
{
    printf("Uso: ./programa [-n tamaño] [-t hilos] [-p]\n");
    printf("Opciones:\n");
    printf("  -n, --tamano     Tamaño de las matrices cuadradas (por defecto: 4)\n");
    printf("  -t, --hilos      Número de hilos a utilizar (por defecto: 2)\n");
    printf("  -p, --imprimir   Imprimir las matrices (opcional)\n");
    printf("  -h, --ayuda      Mostrar esta ayuda\n");
}

int main(int argc, char *argv[])
{
    // Valores por defecto
    int n = 4;         // Tamaño de la matriz
    int num_hilos = 2; // Número de hilos
    int imprimir = 0;  // No imprimir matrices por defecto

    // Definir las opciones para getopt_long
    static struct option opciones_largas[] = {
        {"tamano", required_argument, 0, 'n'},
        {"hilos", required_argument, 0, 't'},
        {"imprimir", no_argument, 0, 'p'},
        {"ayuda", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int opcion;
    int indice_opcion = 0;

    // Procesar los argumentos de la línea de comandos
    while ((opcion = getopt_long(argc, argv, "n:t:ph", opciones_largas, &indice_opcion)) != -1)
    {
        switch (opcion)
        {
        case 'n':
            n = atoi(optarg);
            if (n <= 0)
            {
                fprintf(stderr, "El tamaño de la matriz debe ser positivo\n");
                return EXIT_FAILURE;
            }
            break;
        case 't':
            num_hilos = atoi(optarg);
            if (num_hilos <= 0)
            {
                fprintf(stderr, "El número de hilos debe ser positivo\n");
                return EXIT_FAILURE;
            }
            break;
        case 'p':
            imprimir = 1;
            break;
        case 'h':
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

    // Ajustar el número de hilos si es mayor que el número de filas
    if (num_hilos > n)
    {
        printf("Advertencia: Reduciendo el número de hilos a %d (igual al tamaño de la matriz)\n", n);
        num_hilos = n;
    }

    // Inicializar el generador de números aleatorios
    srand(time(NULL));

    // Crear y llenar las matrices A y B
    int **A = crear_matriz(n);
    int **B = crear_matriz(n);
    int **C = crear_matriz(n);

    llenar_matriz_aleatoria(A, n);
    llenar_matriz_aleatoria(B, n);

    // Registrar el tiempo de inicio
    clock_t inicio = clock();

    // Multiplicar las matrices
    multiplicar_matrices(A, B, C, n, num_hilos);

    // Registrar el tiempo de finalización
    clock_t fin = clock();
    double tiempo_total = (double)(fin - inicio) / CLOCKS_PER_SEC;

    // Imprimir las matrices si se solicitó
    if (imprimir)
    {
        printf("\nMatriz A:\n");
        imprimir_matriz(A, n);

        printf("\nMatriz B:\n");
        imprimir_matriz(B, n);

        printf("\nMatriz Resultado (C = A * B):\n");
        imprimir_matriz(C, n);
    }

    // Imprimir estadísticas
    printf("\nEstadísticas:\n");
    printf("- Tamaño de la matriz: %d x %d\n", n, n);
    printf("- Número de hilos utilizados: %d\n", num_hilos);
    printf("- Tiempo de ejecución: %.6f segundos\n", tiempo_total);

    // Liberar memoria
    liberar_matriz(A, n);
    liberar_matriz(B, n);
    liberar_matriz(C, n);

    return EXIT_SUCCESS;
}