#include <stdio.h>
#include <sys/stat.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>   // Para usar fork y pipe
#include <sys/wait.h> // Para usar wait

#define MAX_CHAR 256
#define MAX_PROCESSES 100

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
char folderName[] = "output_books_pipe";

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
    int allLenght = 0;
    while (index < numberOfBooks && fgets(line, sizeof(line), infile)) {
        int length;
        char name[256];
        sscanf(line, "%s\t%d", name, &length);
       

        allLenght += length;

        books[index].name = strdup(name);
        books[index].length = length;
        //printf("start: %d, end: %d\n", books[index].start, books[index].end);
        

        index++;
    }

    bookSize = index--;
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

    int firstChar = 1;

    for (int i = 0; i < bitStreamLength; i++) {
        if (currentLineCount == 0 && firstChar) {
            books[currentBook].start = i;
            firstChar = 0;
            //printf("i: %d\n", i);
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
                //printf("book: %d, bookSIze: %d\n", currentBook, bookSize);
                //printf("start: %d, end: %d\n\n", books[currentBook].start, books[currentBook].end);
                currentBook++;
                
                if (currentBook >= bookSize) {
                    break;
                }
                currentLineCount = 0;  // Reiniciar el contador de líneas para el próximo libro
                firstChar =1;
            }

            //printf("start: %d, end: %d\n", books[currentBook].start, books[currentBook].end);
                
        }
    }
}

void decodeBook(int bookIndex, int fd[2]) {
    close(fd[0]); // Cerramos la lectura
    
    struct HuffmanNode *current = root;
    int currentLineCount = 0;
    char buffer[256];
    int bufferIndex = 0;

    for (int i = books[bookIndex].start; i < books[bookIndex].end; i++) {
        if (bitStream[i] == '0') {
            current = current->left;
        } else if (bitStream[i] == '1') {
            current = current->right;
        }

        if (current->left == NULL && current->right == NULL) {
            buffer[bufferIndex++] = current->character;

            if (current->character == '\n') {
                currentLineCount++;
            }

            current = root;

            if (currentLineCount >= books[bookIndex].length || bufferIndex >= 255) {
                write(fd[1], buffer, bufferIndex);  // Escribimos lo que decodificamos
                bufferIndex = 0;
                
                if (currentLineCount >= books[bookIndex].length) {
                    printf("start: %d, end: %d\n\n", books[bookIndex].start, books[bookIndex].end);
                
                    break;
                }
            }
        }
    }

    if (bufferIndex > 0) {
        write(fd[1], buffer, bufferIndex);
    }
    close(fd[1]);  
    exit(0);
}

int main() {
    const char *inputFilename = "output_pipe.bin";
    const char *codesFilename = "codes.txt";

    struct timespec start, end;
   
    loadHuffmanCodesFromFile(inputFilename);
    root = buildHuffmanTree(codes, codeSize);

    readCompressedFile(inputFilename, &bitStream, &bitStreamLength, codeSize);
    calculateBitIndex(bitStream, bitStreamLength, root);
    
    mkdir(folderName, 0777);

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < bookSize; i++) {
        int fd[2];  
        pipe(fd);
        pid_t pid = fork();  

        if (pid == 0) {  
            decodeBook(i, fd);
        } else {  // Código del proceso padre
            close(fd[1]); 

            char outputFilename[256];
            snprintf(outputFilename, sizeof(outputFilename), "%s/%s", folderName, books[i].name);
            FILE *outfile = fopen(outputFilename, "w");

            if (outfile == NULL) {
                printf("Error al crear archivo de salida para el libro %d.\n", i);
                exit(1);
            }

            char buffer[256];
            int bytesRead;

            while ((bytesRead = read(fd[0], buffer, sizeof(buffer))) > 0) {
                fwrite(buffer, 1, bytesRead, outfile);  // Escribimos en el archivo desde la tubería
            }

            close(fd[0]);  // Cerramos la lectura en el proceso padre
            fclose(outfile);
        }
    }

    for (int i = 0; i < bookSize; i++) {
        wait(NULL);  
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsedTime = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Tiempo total: %f segundos\n", elapsedTime);

    return 0;
}
