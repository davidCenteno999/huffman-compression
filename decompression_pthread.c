#include <stdio.h>
#include <sys/stat.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>  // Biblioteca para trabajar con hilos

#define MAX_CHAR 256
#define MAX_THREADS 100

struct HuffmanNode {
    char character;
    struct HuffmanNode *left, *right;
};

struct HuffmanCode {
    unsigned int ascii; 
    char *code;
};

struct Books {
    int length; 
    char *name;
    int start;
    int end;
};

struct HuffmanCode codes[MAX_CHAR];
struct Books books[MAX_CHAR];

int codeSize = 0;
int bookSize = 0;
char *bitStream;
int bitStreamLength;
struct HuffmanNode *root;
char folderName[] = "output_books_thread";
pthread_mutex_t fileLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bitStreamLock = PTHREAD_MUTEX_INITIALIZER;

struct HuffmanNode *createNode(char character) {
    struct HuffmanNode *node = (struct HuffmanNode *)malloc(sizeof(struct HuffmanNode));
    node->character = character;
    node->left = node->right = NULL;
    return node;
}

void insertIntoTree(struct HuffmanNode **root, const char *code, char character) {
    struct HuffmanNode *current = *root;
    for (int i = 0; code[i] != '\0'; i++) {
        if (code[i] == '0') {
            if (current->left == NULL) {
                current->left = createNode(0);
            }
            current = current->left;
        } else if (code[i] == '1') {
            if (current->right == NULL) {
                current->right = createNode(0);
            }
            current = current->right;
        }
    }
    current->character = character;
}

struct HuffmanNode *buildHuffmanTree(struct HuffmanCode codes[], int codeSize) {
    struct HuffmanNode *root = createNode(0);
    for (int i = 0; i < codeSize; i++) {
        insertIntoTree(&root, codes[i].code, (char)codes[i].ascii);
    }
    return root;
}

void loadHuffmanCodesFromFile(const char *filename) {
    FILE *infile = fopen(filename, "r");
    if (infile == NULL) {
        printf("Error al abrir el archivo de códigos Huffman.\n");
        exit(1);
    }

    char line[256];
    fgets(line, sizeof(line), infile);
    int numberOfCodes;
    sscanf(line, "%d", &numberOfCodes);

    int index = 0;
    while (index < numberOfCodes && fgets(line, sizeof(line), infile)) {
        unsigned int ascii;
        int length;
        char code[256];
        sscanf(line, "%u\t%d\t%s", &ascii, &length, code);

        codes[index].ascii = ascii;
        codes[index].code = strdup(code);  // Copiar el código
        index++;
    }

    codeSize = index;
    fgets(line, sizeof(line), infile);

    int numberOfBooks;
    sscanf(line, "%d", &numberOfBooks); 

    index = 0;
    while (index < numberOfBooks && fgets(line, sizeof(line), infile)) {
        int length;
        char name[256];
        sscanf(line, "%s\t%d", name, &length);

        books[index].name = strdup(name);
        books[index].length = length;
        //printf("Nombre del libro: %s\n", books[index].name);
        //printf("Longitud del libro (en líneas): %d\n", books[index].length);
        index++;
    }

    bookSize = index;
    fclose(infile);
}

void readCompressedFile(const char *filename, char **bitStream, int *bitStreamLength, int codesSize) {
    FILE *infile = fopen(filename, "rb");
    if (infile == NULL) {
        printf("Error al abrir el archivo comprimido.\n");
        exit(1);
    }

    int numberOfCodes;
    fscanf(infile, "%d\n", &numberOfCodes);

    char line[256];
    for (int i = 0; i < codesSize; i++) {
        fgets(line, sizeof(line), infile);
    }
    int numberOfBooks;
    fscanf(infile, "%d\n", &numberOfBooks);

    for (int i = 0; i < numberOfBooks; i++) {
        fgets(line, sizeof(line), infile);
    }

    long currentPosition = ftell(infile);
    fseek(infile, 0, SEEK_END);
    long endPosition = ftell(infile);
    *bitStreamLength = endPosition - currentPosition;

    fseek(infile, currentPosition, SEEK_SET);
    *bitStream = (char *)malloc(*bitStreamLength);
    if (*bitStream == NULL) {
        printf("Error de memoria.\n");
        exit(1);
    }
    

    fread(*bitStream, 1, *bitStreamLength, infile);
    fclose(infile);
}

void calculateBitIndex(char *bitStream, int bitStreamLength, struct HuffmanNode *root) {
    int currentBook = 0;
    int currentLineCount = 0;
    struct HuffmanNode *current = root;

    for (int i = 0; i < bitStreamLength; i++) {
        if (currentLineCount == 0) {
            books[currentBook].start = i;
        }

        if (bitStream[i] == '0') {
            current = current->left;
        } else if (bitStream[i] == '1') {
            current = current->right;
        }

        if (current->left == NULL && current->right == NULL) {
            if (current->character == '\n') {
                currentLineCount++;
            }

            current = root;

            if (currentLineCount >= books[currentBook].length) {
                books[currentBook].end = i;
                currentBook++;
                if (currentBook >= bookSize) {
                    break;
                }
                currentLineCount = 0;  // Reiniciar el contador de líneas para el próximo libro
            }
        }
    }
}


int bitIndex = 0;

void *decodeBook(void *arg) {

    int bookIndex = *(int *)arg;
    char outputFilename[256];
    int currentLineCount = 0;
    struct HuffmanNode *current = root;


    pthread_mutex_lock(&fileLock);
    snprintf(outputFilename, sizeof(outputFilename), "%s/%s", folderName, books[bookIndex].name);
    FILE *outfile = fopen(outputFilename, "w");
    if (outfile == NULL) {
        printf("Error al crear el archivo descomprimido: %s\n", outputFilename);
        pthread_mutex_unlock(&fileLock);  
        pthread_exit(NULL);
    }
    
    for (int i = books[bookIndex].start; i < books[bookIndex].end; i++) {
        if (bitStream[i] == '0') {
            current = current->left;
        } else if (bitStream[i] == '1') {
            current = current->right;
        }

        if (current->left == NULL && current->right == NULL) {
            
            fputc(current->character, outfile);
             

            if (current->character == '\n') {
                currentLineCount++;
            }

            current = root;

            
            if (currentLineCount >= books[bookIndex].length) {
                break;
            }
        }
    }

    //bookIndex++;
    fclose(outfile);

    pthread_mutex_unlock(&fileLock); 
    
    
    pthread_exit(NULL);
}







int main() {
    const char *inputFilename = "output_thread.bin";
    const char *codesFilename = "codes.txt";

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    loadHuffmanCodesFromFile(inputFilename);
    root = buildHuffmanTree(codes, codeSize);

    readCompressedFile(inputFilename, &bitStream, &bitStreamLength, codeSize);
    calculateBitIndex(bitStream, bitStreamLength, root);
    
    mkdir(folderName, 0777);

    pthread_t threads[MAX_THREADS];
    int threadArgs[MAX_THREADS];

    // Crear un hilo por cada libro
    for (int i = 0; i < bookSize; i++) {
        threadArgs[i] = i;
        pthread_create(&threads[i], NULL, decodeBook, &threadArgs[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < bookSize; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    long elapsed_ns = seconds * 1000000000 + nanoseconds;
    double elapsed_s = elapsed_ns / 1000000000.0;

    printf("El programa tardó %ld nanosegundos en ejecutarse.\n", elapsed_ns);
    printf("El programa tardó %.9f segundos en ejecutarse.\n", elapsed_s);

    printf("Archivo descomprimido exitosamente: output_books_thread\n");

    // Liberar memoria
    free(bitStream);
    for (int i = 0; i < codeSize; i++) {
        free(codes[i].code);
    }
    for (int i = 0; i < bookSize; i++) {
        free(books[i].name);
    }

    return 0;
}
