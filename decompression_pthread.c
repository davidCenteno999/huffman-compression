#include <stdio.h>
#include <sys/stat.h> 
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_CHAR 256
#define MAX_THREADS 4

struct HuffmanNode {
    char character;
    struct HuffmanNode *left, *right;
};

struct HuffmanCode {
    unsigned int ascii; 
    char *code;
};

struct Books {
    int lenght; 
    char *name;
};

struct HuffmanCode codes[MAX_CHAR];

struct Books books[MAX_CHAR];

int codeSize = 0;

int bookSize = 0;


struct ThreadData {
    char *bitStream;
    int start;
    int end;
    struct HuffmanNode *root;
    int thread_id;
    int currentBook;
};


pthread_mutex_t mutex;


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

void *decodeHuffmanThread(void *arg) {
    struct ThreadData *data = (struct ThreadData *)arg;
    struct HuffmanNode *current = data->root;
    int currentLineCount = 0;

    char outputFilename[256];
    snprintf(outputFilename, sizeof(outputFilename), "output_books/%s", books[data->currentBook].name);

    pthread_mutex_lock(&mutex); 
    FILE *outfile = fopen(outputFilename, "a");
    pthread_mutex_unlock(&mutex); 

    if (outfile == NULL) {
        printf("Error al crear el archivo descomprimido: %s\n", outputFilename);
        pthread_exit(NULL);
    }

    for (int i = data->start; i < data->end; i++) {
        if (data->bitStream[i] == '0') {
            current = current->left;
        } else if (data->bitStream[i] == '1') {
            current = current->right;
        }

        if (current->left == NULL && current->right == NULL) {
            pthread_mutex_lock(&mutex); 
            fputc(current->character, outfile);
            pthread_mutex_unlock(&mutex); 

            if (current->character == '\n') {
                currentLineCount++;
            }
            current = data->root;
        }
    }

    fclose(outfile);
    pthread_exit(NULL);
}

void decodeHuffmanConcurrent(char *bitStream, int bitStreamLength, struct HuffmanNode *root) {
    pthread_t threads[MAX_THREADS];
    struct ThreadData threadData[MAX_THREADS];
    int chunkSize = bitStreamLength / MAX_THREADS;

   
    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < MAX_THREADS; i++) {
        threadData[i].bitStream = bitStream;
        threadData[i].start = i * chunkSize;
        threadData[i].end = (i == MAX_THREADS - 1) ? bitStreamLength : (i + 1) * chunkSize;
        threadData[i].root = root;
        threadData[i].thread_id = i;
        threadData[i].currentBook = 0; 
        
        if (pthread_create(&threads[i], NULL, decodeHuffmanThread, &threadData[i]) != 0) {
            printf("Error al crear el hilo %d\n", i);
            exit(1);
        }
    }

    
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
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
        codes[index].code = strdup(code); 
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

        books[index].name =  strdup(name);
        books[index].lenght = length; // Copiar el código
        index++;
    }

    bookSize = index;
    
    

    fclose(infile);

    /*
    printf("Libros cargados:\n");
    for (int i = 0; i < numberOfBooks; i++) {
        printf("i: %s\tLenght: %d\n", books[i].name, books[i].lenght);
    }
    */
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
    (*bitStream)[*bitStreamLength] = '\0'; 

    fclose(infile);
}

int main() {
    const char *inputFilename = "output.bin";
    const char *codesFilename = "codes.txt";

    loadHuffmanCodesFromFile(codesFilename);
    struct HuffmanNode *root = buildHuffmanTree(codes, codeSize);

    char *bitStream;
    int bitStreamLength;
    readCompressedFile(inputFilename, &bitStream, &bitStreamLength, codeSize);

    decodeHuffmanConcurrent(bitStream, bitStreamLength, root);

    free(bitStream);
    for (int i = 0; i < codeSize; i++) {
        free(codes[i].code);
    }

    return 0;
}

