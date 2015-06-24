#include <stdio.h>
#include "./include/t2fs.h"

void ls(DIR2 dirHandle)
{
    DIRENT2 dirEntry;

    while(readdir2(dirHandle, &dirEntry) >= 0) {
        printf("Name: %s Type: %d Size: %lu\n", dirEntry.name, dirEntry.fileType, dirEntry.fileSize);
    }
}

int main(int argc, char *argv[]) {
    int handleArq1 = create2("Arquivo1");
    int handleArq2 = create2("Arquivo2");
    int handleArq3 = create2("Arquivo2");
    if (handleArq3 >= 0)
        printf("Erro não identificado! Nao deve permitir arquivos iguais\n");

    handleArq3 = create2("Nomeinválido");
    if (handleArq3 >= 0)
        printf("Erro não identificado! Nao deve permitir nome invalido\n");

    handleArq3 = create2("Nomeinvalidopqehmuitograndedemaistipoassimgrandedemaisbagarai");
    if (handleArq3 >= 0)
        printf("Erro não identificado! Nao deve permitir nome muito grande\n");

    handleArq3 = create2("./././arquivo3");
    if (handleArq3 < 0)
        printf("Erro não identificado! Deve permitir nome de arquivo com barras?\n");

    DIR2 handle = opendir2("/");
    ls(handle);
    return 0;
}