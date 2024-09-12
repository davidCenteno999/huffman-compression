#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHAR 256

struct HuffmanNode {
    char character;
    struct HuffmanNode *left, *right;
};

struct HuffmanCode {
    unsigned int ascii; // Guardar valor ASCII en lugar de carácter
    char *code;
};

struct HuffmanCode codes[MAX_CHAR];
int codeSize = 0;

// Crear un nuevo nodo para el árbol de Huffman
struct HuffmanNode *createNode(char character) {
    struct HuffmanNode *node = (struct HuffmanNode *)malloc(sizeof(struct HuffmanNode));
    node->character = character;
    node->left = node->right = NULL;
    return node;
}

// Insertar un código en el árbol de Huffman
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

// Construir el árbol de Huffman a partir de los códigos
struct HuffmanNode *buildHuffmanTree(struct HuffmanCode codes[], int codeSize) {
    struct HuffmanNode *root = createNode(0);
    for (int i = 0; i < codeSize; i++) {
        insertIntoTree(&root, codes[i].code, (char)codes[i].ascii);
    }
    return root;
}

// Leer códigos Huffman desde un archivo
void loadHuffmanCodesFromFile(const char *filename) {
    FILE *infile = fopen(filename, "r");
    if (infile == NULL) {
        printf("Error al abrir el archivo de códigos Huffman.\n");
        exit(1);
    }

    char line[256];
    
    // Leer la primera línea para obtener la cantidad de códigos
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

    fclose(infile);

    printf("Códigos Huffman cargados:\n");
    for (int i = 0; i < codeSize; i++) {
        printf("ASCII: %u\tCódigo: %s\n", codes[i].ascii, codes[i].code);
    }
}

// Leer el archivo comprimido en un flujo de bits
void readCompressedFile(const char *filename, char **bitStream, int *bitStreamLength, int codesSize) {
    FILE *infile = fopen(filename, "rb");
    if (infile == NULL) {
        printf("Error al abrir el archivo comprimido.\n");
        exit(1);
    }

    // Leer la cantidad de códigos Huffman
    int numberOfCodes;
    fscanf(infile, "%d\n", &numberOfCodes); // Leer la cantidad de códigos en la primera línea

    // Leer los códigos Huffman (esto debe ser consistente con la función que guarda los códigos)
    char line[256];
    for (int i = 0; i < codesSize; i++) {
        fgets(line, sizeof(line), infile);
    }

    // Obtener la posición actual después de leer los códigos Huffman
    long currentPosition = ftell(infile);

    // Mover al final del archivo para obtener la longitud total
    fseek(infile, 0, SEEK_END);
    long endPosition = ftell(infile);

    // Imprimir la posición actual y la posición final
    printf("Posición actual: %ld\n", currentPosition);
    printf("Posición final: %ld\n", endPosition);

    // Calcular el tamaño del bitstream
    *bitStreamLength = endPosition - currentPosition;
    
    // Mover el puntero a la posición actual para comenzar a leer el bitstream
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


// Decodificar el flujo de bits usando el árbol de Huffman
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

        if (current->left == NULL && current->right == NULL) { // Nodo hoja
            fputc(current->character, outfile);
            current = root; // Restablecer al nodo raíz para el siguiente carácter
        }
    }

    fclose(outfile);
}

// Función principal
int main() {
    const char *inputFilename = "output.bin";
    const char *codesFilename = "codes.txt";
    const char *decompressedFilename = "decompressed.txt";

    loadHuffmanCodesFromFile(inputFilename);

    struct HuffmanNode *root = buildHuffmanTree(codes, codeSize);

    char *bitStream;
    int bitStreamLength;
    readCompressedFile(inputFilename, &bitStream, &bitStreamLength, codeSize);

    decodeHuffman(bitStream, bitStreamLength, decompressedFilename, root);

    printf("Archivo descomprimido creado exitosamente: %s\n", decompressedFilename);

    free(bitStream);
    for (int i = 0; i < codeSize; i++) {
        free(codes[i].code);
    }

    return 0;
}
