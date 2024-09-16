#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <time.h>

#define MAX_TREE_HT 3000
#define MAX_CHAR 256




struct MinHeapNode {
    char data;
    unsigned freq;
    struct MinHeapNode *left, *right;
};

struct MinHeap {
    unsigned size;
    unsigned capacity;
    struct MinHeapNode** array;
};

struct HuffmanCode {
    char character;
    char *code;
};

struct BookInfo {
    char name[200];
    int lineCount;
};

char *text = NULL;
int textLength = 0, textCapacity = 0;
int freq[MAX_CHAR] = {0};
char characters[MAX_CHAR];
int charIndex = 0;
struct BookInfo books[100];
int bookCount = 0;

struct ThreadData {
    int localFreq[MAX_CHAR];
    char localCharacters[MAX_CHAR];
    int localCharIndex;
    char *localText;
    int localTextLength;
    int localTextCapacity;
    struct BookInfo localBooks[100];
    int localBookCount;
};


pthread_mutex_t lock;

// Funciones para manejo del Árbol de Huffman (como en tu código original)

struct MinHeapNode* newNode(char data, unsigned freq) {
    struct MinHeapNode* temp = (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode));
    temp->left = temp->right = NULL;
    temp->data = data;
    temp->freq = freq;
    return temp;
}

struct MinHeap* createMinHeap(unsigned capacity) {
    struct MinHeap* minHeap = (struct MinHeap*)malloc(sizeof(struct MinHeap));
    minHeap->size = 0;
    minHeap->capacity = capacity;
    minHeap->array = (struct MinHeapNode**)malloc(minHeap->capacity * sizeof(struct MinHeapNode*));
    return minHeap;
}

void swapMinHeapNode(struct MinHeapNode** a, struct MinHeapNode** b) {
    struct MinHeapNode* t = *a;
    *a = *b;
    *b = t;
}

void minHeapify(struct MinHeap* minHeap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < minHeap->size && minHeap->array[left]->freq < minHeap->array[smallest]->freq)
        smallest = left;

    if (right < minHeap->size && minHeap->array[right]->freq < minHeap->array[smallest]->freq)
        smallest = right;

    if (smallest != idx) {
        swapMinHeapNode(&minHeap->array[smallest], &minHeap->array[idx]);
        minHeapify(minHeap, smallest);
    }
}

int isSizeOne(struct MinHeap* minHeap) {
    return (minHeap->size == 1);
}

struct MinHeapNode* extractMin(struct MinHeap* minHeap) {
    struct MinHeapNode* temp = minHeap->array[0];
    minHeap->array[0] = minHeap->array[minHeap->size - 1];
    --minHeap->size;
    minHeapify(minHeap, 0);
    return temp;
}

void insertMinHeap(struct MinHeap* minHeap, struct MinHeapNode* minHeapNode) {
    ++minHeap->size;
    int i = minHeap->size - 1;

    while (i && minHeapNode->freq < minHeap->array[(i - 1) / 2]->freq) {
        minHeap->array[i] = minHeap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }

    minHeap->array[i] = minHeapNode;
}

void buildMinHeap(struct MinHeap* minHeap) {
    int n = minHeap->size - 1;
    for (int i = (n - 1) / 2; i >= 0; --i)
        minHeapify(minHeap, i);
}

int isLeaf(struct MinHeapNode* root) {
    return !(root->left) && !(root->right);
}

struct MinHeap* createAndBuildMinHeap(char data[], int freq[], int size) {
    struct MinHeap* minHeap = createMinHeap(size);
    for (int i = 0; i < size; ++i) {
        minHeap->array[i] = newNode(data[i], freq[(unsigned char)data[i]]);
    }
    minHeap->size = size;
    buildMinHeap(minHeap);
    return minHeap;
}

struct MinHeapNode* buildHuffmanTree(char data[], int freq[], int size) {
    struct MinHeapNode *left, *right, *top;
    struct MinHeap* minHeap = createAndBuildMinHeap(data, freq, size);

    while (!isSizeOne(minHeap)) {
        left = extractMin(minHeap);
        right = extractMin(minHeap);
        top = newNode('$', left->freq + right->freq);
        top->left = left;
        top->right = right;
        insertMinHeap(minHeap, top);
    }

    return extractMin(minHeap);
}

void storeCodes(struct MinHeapNode* root, int arr[], int top, struct HuffmanCode codes[], int *index) {
    if (root->left) {
        arr[top] = 0;
        storeCodes(root->left, arr, top + 1, codes, index);
    }

    if (root->right) {
        arr[top] = 1;
        storeCodes(root->right, arr, top + 1, codes, index);
    }

    if (isLeaf(root)) {
        codes[*index].character = root->data;
        codes[*index].code = (char*)malloc(top + 1);
        for (int i = 0; i < top; i++) {
            codes[*index].code[i] = arr[i] + '0';
        }
        codes[*index].code[top] = '\0';
        (*index)++;
    }
}

void HuffmanCodes(char data[], int freq[], int size, struct HuffmanCode codes[], int *codeSize) {
    struct MinHeapNode* root = buildHuffmanTree(data, freq, size);
    int arr[MAX_TREE_HT], top = 0, index = 0;
    storeCodes(root, arr, top, codes, &index);
    *codeSize = index;
}

void writeCompressedFile(const char *filename, struct HuffmanCode codes[], char *text, int textLength, int codeSize, struct BookInfo books[], int bookCount) {
    FILE *outfile = fopen(filename, "wb");
    if (outfile == NULL) {
        printf("Error al crear el archivo comprimido.\n");
        exit(1);
    }

    fprintf(outfile, "%d\n", codeSize);
    for (int i = 0; i < codeSize; i++) {
        fprintf(outfile, "%d\t%d\t%s\n", (unsigned char)codes[i].character, (int)strlen(codes[i].code), codes[i].code);
    }

    fprintf(outfile, "%d\n", bookCount); 
    for (int i = 0; i < bookCount; i++) {
        fprintf(outfile, "%s\t%d\n", books[i].name, books[i].lineCount);
    }

    for (int i = 0; i < textLength; i++) {
        for (int j = 0; j < MAX_CHAR; j++) {
            if (codes[j].character == text[i]) {
                fprintf(outfile, "%s", codes[j].code);
                break;
            }
        }
    }

    fclose(outfile);
}

void *processFile(void *arg) {
    char *filepath = (char*)arg;
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        printf("No se puede abrir el archivo %s\n", filepath);
        free(filepath); 
        pthread_exit(NULL);
    }

    // Inicializar datos locales
    struct ThreadData threadData = {0};
    threadData.localText = NULL;
    threadData.localTextLength = 0;
    threadData.localTextCapacity = 0;
    threadData.localCharIndex = 0;
    threadData.localBookCount = 0;

    char buffer[4096];
    size_t bytesRead;
    int lineCount = 0;

    
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            char ch = buffer[i];
            if (ch == '\n') {
                lineCount++;
            }

            if (threadData.localFreq[(unsigned char)ch] == 0) {
                threadData.localCharacters[threadData.localCharIndex++] = ch;
            }
            threadData.localFreq[(unsigned char)ch]++;

           
            if (threadData.localTextLength + 1 > threadData.localTextCapacity) {
                threadData.localTextCapacity = (threadData.localTextCapacity == 0) ? 1 : threadData.localTextCapacity * 2;
                char *newText = realloc(threadData.localText, threadData.localTextCapacity * sizeof(char));
                if (!newText) {
                    free(threadData.localText);
                    fclose(file);
                    free(filepath);
                    pthread_exit(NULL);  
                }
                threadData.localText = newText;
            }
            threadData.localText[threadData.localTextLength++] = ch;
        }
    }

    fclose(file);

    
    char *filename = strrchr(filepath, '/');
    if (filename == NULL) {
        filename = filepath;   
    } else {
        filename++;  
    }

    
    snprintf(threadData.localBooks[threadData.localBookCount].name, sizeof(threadData.localBooks[threadData.localBookCount].name), "%s", filename);
    threadData.localBooks[threadData.localBookCount].lineCount = lineCount;
    threadData.localBookCount++;

    // Fusionar datos locales en los datos globales (sección crítica protegida por mutex)
    pthread_mutex_lock(&lock);

    for (int i = 0; i < MAX_CHAR; i++) {
        freq[i] += threadData.localFreq[i];
    }

    for (int i = 0; i < threadData.localCharIndex; i++) {
        int found = 0;
        for (int j = 0; j < charIndex; j++) {
            if (characters[j] == threadData.localCharacters[i]) {
                found = 1;
                break;
            }
        }
        if (!found) {
            characters[charIndex++] = threadData.localCharacters[i];
        }
    }

    if (textLength + threadData.localTextLength > textCapacity) {
        textCapacity = textLength + threadData.localTextLength;
        char *newText = realloc(text, textCapacity * sizeof(char));
        if (!newText) {
            free(text);
            pthread_mutex_unlock(&lock);
            free(threadData.localText);
            free(filepath);
            pthread_exit(NULL);  
        }
        text = newText;
    }

    memcpy(text + textLength, threadData.localText, threadData.localTextLength * sizeof(char));
    textLength += threadData.localTextLength;

    for (int i = 0; i < threadData.localBookCount; i++) {
        snprintf(books[bookCount].name, sizeof(books[bookCount].name), "%s", threadData.localBooks[i].name);
        books[bookCount].lineCount = threadData.localBooks[i].lineCount;
        bookCount++;
    }

    pthread_mutex_unlock(&lock);  

    
    free(threadData.localText);
    free(filepath);

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[100];
    int threadCount = 0;
    char outputFilename[100] = "output_thread.bin";

    struct dirent *entry;
    DIR *dir;
    char directory[] = "libros";

    dir = opendir(directory);
    if (dir == NULL) {
        printf("No se puede abrir la carpeta %s\n", directory);
        return 1;
    }

    pthread_mutex_init(&lock, NULL);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Crear hilos para cada archivo en el directorio
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);
            pthread_create(&threads[threadCount++], NULL, processFile, strdup(filepath));
        }
    }

    closedir(dir);

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL);
    }

    

    struct HuffmanCode codes[MAX_CHAR];
    int codeSize = 0;
    HuffmanCodes(characters, freq, charIndex, codes, &codeSize);

    writeCompressedFile(outputFilename, codes, text, textLength, codeSize, books, bookCount);

    clock_gettime(CLOCK_MONOTONIC, &end);

    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    long elapsed_ns = seconds * 1000000000 + nanoseconds;
    double elapsed_s = elapsed_ns / 1000000000.0;

    printf("El programa tardó %ld nanosegundos en ejecutarse.\n", elapsed_ns);
    printf("El programa tardó %.9f segundos en ejecutarse.\n", elapsed_s);
    printf("Archivo comprimido creado exitosamente: %s\n", outputFilename);

    pthread_mutex_destroy(&lock);
    free(text);

    return 0;
}
