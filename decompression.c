#include <stdio.h>
#include <sys/stat.h> 
#include <stdlib.h>
#include <string.h>

#define MAX_CHAR 256

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
        codes[index].code = strdup(code); // Copiar el código
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


void decodeHuffman(char *bitStream, int bitStreamLength, const char *outputFilename, struct HuffmanNode *root) {
    
    FILE *outfile = fopen(outputFilename, "w");
    if (outfile == NULL) {
        printf("Error al crear el archivo descomprimido.\n");
        exit(1);
    }

    struct HuffmanNode *current = root;
    for (int i = 0; i < bitStreamLength; i++) {
        if (bitStream[i] == '0') {
            current = current->left;
        } else if (bitStream[i] == '1') {
            current = current->right;
        }

        if (current->left == NULL && current->right == NULL) {
            fputc(current->character, outfile);
            current = root; 
        }
    }

    fclose(outfile);
}

void decodeHuffmanMultipleFiles(char *bitStream, int bitStreamLength, struct HuffmanNode *root) {
    int currentBook = 0;  
    int currentLineCount = 0;  
    char outputFilename[256];  
    char folderName[] = "output_books";  

    
    if (mkdir(folderName, 0777) == -1) {
        printf("Error al crear la carpeta o ya existe: %s\n", folderName);
    } else {
        //printf("Carpeta creada: %s\n", folderName);
    }

    snprintf(outputFilename, sizeof(outputFilename), "%s/%s", folderName, books[currentBook].name);
    FILE *outfile = fopen(outputFilename, "w");
    if (outfile == NULL) {
        printf("Error al crear el archivo descomprimido: %s\n", outputFilename);
        exit(1);
    }

    struct HuffmanNode *current = root;
    for (int i = 0; i < bitStreamLength; i++) {
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
            if (currentLineCount >= books[currentBook].lenght) {
                fclose(outfile);  
                currentBook++;  

                if (currentBook >= bookSize) {
                    break;
                }
                snprintf(outputFilename, sizeof(outputFilename), "%s/%s", folderName, books[currentBook].name);
                outfile = fopen(outputFilename, "w");
                if (outfile == NULL) {
                    printf("Error al crear el archivo descomprimido: %s\n", outputFilename);
                    exit(1);
                }

                currentLineCount = 0; 
            }
        }
    }

    // Cerrar archivo
    if (outfile != NULL) {
        fclose(outfile);
    }
}


int main() {
    const char *inputFilename = "output.bin";
    const char *codesFilename = "codes.txt";
    const char *decompressedFilename = "decompressed.txt";

    loadHuffmanCodesFromFile(inputFilename);

    struct HuffmanNode *root = buildHuffmanTree(codes, codeSize);

    char *bitStream;
    int bitStreamLength;
    readCompressedFile(inputFilename, &bitStream, &bitStreamLength, codeSize);

    //decodeHuffman(bitStream, bitStreamLength, decompressedFilename, root);

    decodeHuffmanMultipleFiles(bitStream, bitStreamLength, root);

    //printf("Archivo descomprimido creado exitosamente: %s\n", decompressedFilename);

    free(bitStream);
    for (int i = 0; i < codeSize; i++) {
        free(codes[i].code);
    }

    return 0;
}
