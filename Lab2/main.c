#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

// Structure to hold matrix dimensions and data
typedef struct
{
    int rows;
    int cols;
    int **data;
} Matrix;

// Structure to pass data to thread functions
typedef struct
{
    Matrix *A;
    Matrix *B;
    Matrix *C1;
    Matrix *C2;
    Matrix *C3;
    int row;
    int col;
} ThreadData;

// Structure to return time and number of threads used
typedef struct
{
    double time;
    int threads;
} results;

// Default filenames
char *filenames[] = {"a", "b", "c"};

// Function prototypes
ThreadData *parseInput(int argc, char *argv[]);
Matrix *read_matrix_from_file(const char *filename);
Matrix *create_matrix(int rows, int cols);
results multiply_per_matrix(ThreadData *data);
results multiply_per_row(ThreadData *data);
void *thread_Multiply_Row(void *data);
results multiply_per_element(ThreadData *data);
void *thread_Multiply_Element(void *thread_data);
void outputMatrices(char *filenames[], ThreadData *data);
void write_matrix_to_file(Matrix *matrix, const char *filename);
void free_matrix(Matrix *matrix);
void cleanup(ThreadData *data);

ThreadData *parseInput(int argc, char *argv[])
{
    if (argc >= 4)
    {
        filenames[0] = argv[1];
        filenames[1] = argv[2];
        filenames[2] = argv[3];
    }

    // Read matrices A and B from files
    Matrix *A = read_matrix_from_file(filenames[0]);
    Matrix *B = read_matrix_from_file(filenames[1]);

    // Create thread data
    ThreadData *data = malloc(sizeof(ThreadData));
    if (data == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for thread data\n");
        exit(EXIT_FAILURE);
    }

    // Add matrices A, B, and output matrix C to thread data
    data->A = A;
    data->B = B;
    data->C1 = create_matrix(A->rows, B->cols);
    data->C2 = create_matrix(A->rows, B->cols);
    data->C3 = create_matrix(A->rows, B->cols);

    return data;
}

Matrix *read_matrix_from_file(const char *filename)
{
    // Implementation to read a matrix from a file
    char *modifiedFilename = malloc(strlen(filename) + 4);
    strcpy(modifiedFilename, filename);
    strcat(modifiedFilename, ".txt");
    FILE *file = fopen(modifiedFilename, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        exit(EXIT_FAILURE);
    }
    free(modifiedFilename);

    // Read the number of rows and columns from the file
    int rows, cols;
    fscanf(file, "  row=%d col=%d", &rows, &cols);

    // Create the matrix
    Matrix *newMatrix = create_matrix(rows, cols);

    // Read the matrix elements from the file
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            fscanf(file, "%d", &(newMatrix->data[i][j]));
        }
    }

    // Close the file
    fclose(file);
    return newMatrix;
}

Matrix *create_matrix(int rows, int cols)
{
    // Implementation to create a matrix with the given dimensions
    Matrix *matrix = malloc(sizeof(Matrix));
    if (matrix == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for matrix\n");
        exit(EXIT_FAILURE);
    }

    matrix->rows = rows;
    matrix->cols = cols;

    matrix->data = malloc(rows * sizeof(int *));
    if (matrix->data == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for matrix data\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows; i++)
    {
        matrix->data[i] = malloc(cols * sizeof(int));
        if (matrix->data[i] == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for matrix row\n");
            exit(EXIT_FAILURE);
        }
    }
    return matrix;
}

results multiply_per_matrix(ThreadData *data)
{
    clock_t start_time = clock();

    // Calculate the multiplication result of matrices A and B
    for (int i = 0; i < data->C1->rows; i++)
    {
        for (int j = 0; j < data->B->cols; j++)
        {
            int sum = 0;
            for (int k = 0; k < data->A->cols; k++)
            {
                sum += data->A->data[i][k] * data->B->data[k][j];
            }
            data->C1->data[i][j] = sum;
        }
    }

    clock_t end_time = clock();
    double execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    results result;
    result.threads = 1;
    result.time = execution_time;
    return result;
}

results multiply_per_row(ThreadData *data)
{
    clock_t start_time = clock();

    // Create an array of threads
    pthread_t *threads = malloc(data->C2->rows * sizeof(pthread_t));
    if (threads == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for threads\n");
        exit(EXIT_FAILURE);
    }

    // Create thread data for each row
    ThreadData *row_data = malloc(data->C2->rows * sizeof(ThreadData));
    if (row_data == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for row data\n");
        exit(EXIT_FAILURE);
    }

    // Create threads to calculate each row of matrix C2
    for (int i = 0; i < data->C2->rows; i++)
    {
        row_data[i].A = data->A;
        row_data[i].B = data->B;
        row_data[i].C2 = data->C2;
        row_data[i].row = i;
        row_data[i].col = 0;

        if (pthread_create(&threads[i], NULL, thread_Multiply_Row, &row_data[i]) != 0)
        {
            fprintf(stderr, "Failed to create thread\n");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < data->C2->rows; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            fprintf(stderr, "Failed to join thread\n");
            exit(EXIT_FAILURE);
        }
    }

    // Calculate the execution time
    clock_t end_time = clock();
    double execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    // Free the memory allocated for threads and row data
    free(threads);
    free(row_data);

    // Create the results structure
    results result;
    result.threads = data->C2->rows;
    result.time = execution_time;

    return result;
}

void *thread_Multiply_Row(void *thread_data)
{
    // Implementation for a thread per row
    ThreadData *data = (ThreadData *)thread_data;
    for (int j = 0; j < data->B->cols; j++)
    {
        int sum = 0;
        for (int k = 0; k < data->A->cols; k++)
        {
            sum += data->A->data[data->row][k] * data->B->data[k][j];
        }
        data->C2->data[data->row][j] = sum;
    }
}

results multiply_per_element(ThreadData *data)
{
    clock_t start_time = clock();

    // Create an array of threads
    pthread_t *threads = malloc(data->C3->rows * data->C3->cols * sizeof(pthread_t));
    if (threads == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for threads\n");
        exit(EXIT_FAILURE);
    }

    // Create thread data for each element
    ThreadData *element_data = malloc(data->C3->rows * data->C3->cols * sizeof(ThreadData));
    if (element_data == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for element data\n");
        exit(EXIT_FAILURE);
    }

    // Create threads to calculate each element of matrix C3
    int thread_index = 0;
    for (int i = 0; i < data->C3->rows; i++)
    {
        for (int j = 0; j < data->C3->cols; j++)
        {
            element_data[thread_index].A = data->A;
            element_data[thread_index].B = data->B;
            element_data[thread_index].C3 = data->C3;
            element_data[thread_index].row = i;
            element_data[thread_index].col = j;

            if (pthread_create(&threads[thread_index], NULL, thread_Multiply_Element, &element_data[thread_index]) != 0)
            {
                fprintf(stderr, "Failed to create thread\n");
                exit(EXIT_FAILURE);
            }

            thread_index++;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < data->C3->rows * data->C3->cols; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            fprintf(stderr, "Failed to join thread\n");
            exit(EXIT_FAILURE);
        }
    }

    // Calculate the execution time
    clock_t end_time = clock();
    double execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    // Free the memory allocated for threads and element data
    free(threads);
    free(element_data);

    // Create the results structure
    results result;
    result.threads = data->C3->rows * data->C3->cols;
    result.time = execution_time;

    return result;
}

void *thread_Multiply_Element(void *thread_data)
{
    // Implementation for a thread per element
    ThreadData *data = (ThreadData *)thread_data;
    int sum = 0;
    for (int k = 0; k < data->A->cols; k++)
    {
        sum += data->A->data[data->row][k] * data->B->data[k][data->col];
    }
    data->C3->data[data->row][data->col] = sum;
}

void outputMatrices(char *filenames[], ThreadData *data)
{
    // Write the output matrices to files
    char *modifiedFilename = malloc(strlen(filenames[2]) + 25);
    strcpy(modifiedFilename, filenames[2]);
    strcat(modifiedFilename, "_using_thread_per_matrix");
    write_matrix_to_file(data->C1, modifiedFilename);

    strcpy(modifiedFilename, filenames[2]);
    strcat(modifiedFilename, "_using_thread_per_row");
    write_matrix_to_file(data->C2, modifiedFilename);

    strcpy(modifiedFilename, filenames[2]);
    strcat(modifiedFilename, "_using_thread_per_element");
    write_matrix_to_file(data->C3, modifiedFilename);

    free(modifiedFilename);
}

void write_matrix_to_file(Matrix *matrix, const char *filename)
{
    // Implementation to write a matrix to a file
    char *modifiedFilename = malloc(strlen(filename) + 4);
    strcpy(modifiedFilename, filename);
    strcat(modifiedFilename, ".txt");
    FILE *file = fopen(modifiedFilename, "w");
    if (file == NULL)
    {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        exit(EXIT_FAILURE);
    }
    free(modifiedFilename);

    // Write the number of rows and columns to the file
    fprintf(file, "row=%d col=%d\n", matrix->rows, matrix->cols);

    // Write the matrix elements to the file
    for (int i = 0; i < matrix->rows; i++)
    {
        for (int j = 0; j < matrix->cols; j++)
        {
            fprintf(file, "%d ", matrix->data[i][j]);
        }
        fprintf(file, "\n");
    }

    // Close the file
    fclose(file);
}

void free_matrix(Matrix *matrix)
{
    // Implementation to free the memory allocated for a matrix
    for (int i = 0; i < matrix->rows; i++)
    {
        free(matrix->data[i]);
    }
    free(matrix->data);
    free(matrix);
}

void cleanup(ThreadData *data)
{
    // Free the memory allocated for the matrices and thread data
    free_matrix(data->A);
    free_matrix(data->B);
    free_matrix(data->C1);
    free_matrix(data->C2);
    free_matrix(data->C3);
    free(data);
}

int main(int argc, char *argv[])
{
    // Parse command line arguments
    ThreadData *data = parseInput(argc, argv);

    // Run the three methods and output the results
    // Method 1
    results result1 = multiply_per_matrix(data);
    printf("Time taken by first method: %lf\n", result1.time);
    printf("Number of threads used by first method: %d\n", result1.threads);

    // Method 2
    results result2 = multiply_per_row(data);
    printf("Time taken by second method: %lf\n", result2.time);
    printf("Number of threads used by second method: %d\n", result2.threads);

    // Method 3
    results result3 = multiply_per_element(data);
    printf("Time taken by third method: %lf\n", result3.time);
    printf("Number of threads used by third method: %d\n", result3.threads);

    // Write the output matrices to files
    outputMatrices(filenames, data);
    cleanup(data);
}
