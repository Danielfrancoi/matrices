#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <getopt.h>
#include <string.h>

// Prototipos de funciones
int **crear_matriz_compartida(int n, const char *nombre);
void llenar_matriz_aleatoria(int **matriz, int n);
void imprimir_matriz(int **matriz, int n);
void liberar_matriz_compartida(int **matriz, int n, const char *nombre);
void multiplicar_matrices_proceso(int **A, int **B, int **C, int n, int fila_inicio, int fila_fin);
void multiplicar_matrices(int **A, int **B, int **C, int n, int num_procesos);

// Función para crear una matriz compartida usando memoria mapeada
int **crear_matriz_compartida(int n, const char *nombre)
{
    int shm_fd;
    int **matriz;
    int total_size = n * sizeof(int *) + n * n * sizeof(int);

    // Crear o abrir el objeto de memoria compartida
    shm_fd = shm_open(nombre, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error al abrir la memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Establecer el tamaño del objeto de memoria compartida
    if (ftruncate(shm_fd, total_size) == -1)
    {
        perror("Error al establecer el tamaño de la memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Mapear el objeto de memoria compartida en el espacio de direcciones del proceso
    void *ptr = mmap(0, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("Error al mapear la memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Configurar la matriz
    matriz = (int **)ptr;
    int *data = (int *)((char *)ptr + n * sizeof(int *));

    for (int i = 0; i < n; i++)
    {
        matriz[i] = data + i * n;
    }

    close(shm_fd);
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

// Función para liberar la memoria de una matriz compartida
void liberar_matriz_compartida(int **matriz, int n, const char *nombre)
{
    int total_size = n * sizeof(int *) + n * n * sizeof(int);

    if (munmap((void *)matriz, total_size) == -1)
    {
        perror("Error al desmapear la memoria compartida");
    }

    if (shm_unlink(nombre) == -1)
    {
        perror("Error al desvincular la memoria compartida");
    }
}

// Función para multiplicar una porción de las matrices
void multiplicar_matrices_proceso(int **A, int **B, int **C, int n, int fila_inicio, int fila_fin)
{
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
}

// Función para multiplicar matrices utilizando procesos
void multiplicar_matrices(int **A, int **B, int **C, int n, int num_procesos)
{
    pid_t pid;
    int filas_por_proceso = n / num_procesos;
    int filas_restantes = n % num_procesos;
    int fila_actual = 0;

    for (int i = 0; i < num_procesos; i++)
    {
        // Calcular el rango de filas para este proceso
        int filas_este_proceso = filas_por_proceso;
        if (filas_restantes > 0)
        {
            filas_este_proceso++;
            filas_restantes--;
        }

        int fila_inicio = fila_actual;
        fila_actual += filas_este_proceso;
        int fila_fin = fila_actual;

        // Crear un nuevo proceso
        pid = fork();

        if (pid < 0)
        {
            // Error al crear el proceso
            fprintf(stderr, "Error al crear el proceso %d\n", i);
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // Código del proceso hijo
            multiplicar_matrices_proceso(A, B, C, n, fila_inicio, fila_fin);
            exit(EXIT_SUCCESS);
        }
        // El proceso padre continúa creando más procesos hijos
    }

    // El proceso padre espera a que todos los procesos hijos terminen
    for (int i = 0; i < num_procesos; i++)
    {
        wait(NULL);
    }
}

void mostrar_ayuda()
{
    printf("Uso: ./programa [-n tamaño] [-p procesos] [-i]\n");
    printf("Opciones:\n");
    printf("  -n, --tamano     Tamaño de las matrices cuadradas (por defecto: 4)\n");
    printf("  -p, --procesos   Número de procesos a utilizar (por defecto: 2)\n");
    printf("  -i, --imprimir   Imprimir las matrices (opcional)\n");
    printf("  -h, --ayuda      Mostrar esta ayuda\n");
}

int main(int argc, char *argv[])
{
    // Valores por defecto
    int n = 4;            // Tamaño de la matriz
    int num_procesos = 2; // Número de procesos
    int imprimir = 0;     // No imprimir matrices por defecto

    // Definir las opciones para getopt_long
    static struct option opciones_largas[] = {
        {"tamano", required_argument, 0, 'n'},
        {"procesos", required_argument, 0, 'p'},
        {"imprimir", no_argument, 0, 'i'},
        {"ayuda", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int opcion;
    int indice_opcion = 0;

    // Procesar los argumentos de la línea de comandos
    while ((opcion = getopt_long(argc, argv, "n:p:ih", opciones_largas, &indice_opcion)) != -1)
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
        case 'p':
            num_procesos = atoi(optarg);
            if (num_procesos <= 0)
            {
                fprintf(stderr, "El número de procesos debe ser positivo\n");
                return EXIT_FAILURE;
            }
            break;
        case 'i':
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

    // Ajustar el número de procesos si es mayor que el número de filas
    if (num_procesos > n)
    {
        printf("Advertencia: Reduciendo el número de procesos a %d (igual al tamaño de la matriz)\n", n);
        num_procesos = n;
    }

    // Inicializar el generador de números aleatorios
    srand(time(NULL));

    // Crear y llenar las matrices A y B usando memoria compartida
    int **A = crear_matriz_compartida(n, "/matriz_A");
    int **B = crear_matriz_compartida(n, "/matriz_B");
    int **C = crear_matriz_compartida(n, "/matriz_C");

    llenar_matriz_aleatoria(A, n);
    llenar_matriz_aleatoria(B, n);

    // Registrar el tiempo de inicio
    clock_t inicio = clock();

    // Multiplicar las matrices usando procesos
    multiplicar_matrices(A, B, C, n, num_procesos);

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
    printf("- Número de procesos utilizados: %d\n", num_procesos);
    printf("- Tiempo de ejecución: %.6f segundos\n", tiempo_total);

    // Liberar memoria compartida
    liberar_matriz_compartida(A, n, "/matriz_A");
    liberar_matriz_compartida(B, n, "/matriz_B");
    liberar_matriz_compartida(C, n, "/matriz_C");

    return EXIT_SUCCESS;
}