#include <stdio.h>
#include <string.h>

int main() {
    FILE *file = fopen("output.bin", "wb");

    // Escribir texto al archivo
    const char *text = "Este es un archivo con texto y binario.";
    fwrite(text, sizeof(char), strlen(text), file);

    // Escribir datos binarios
    int binaryData = 12345;  // Datos binarios de ejemplo
    fwrite(&binaryData, sizeof(int), 1, file);

    fclose(file);

    // Leer el archivo
    file = fopen("output.bin", "rb");

    // Leer el texto
    char buffer[100];
    fread(buffer, sizeof(char), strlen(text), file);
    buffer[strlen(text)] = '\0';  // Asegurarse de terminar el string
    printf("Texto leído: %s\n", buffer);

    // Leer los datos binarios
    fread(&binaryData, sizeof(int), 1, file);
    printf("Datos binarios leídos: %d\n", binaryData);

    fclose(file);
    return 0;
}