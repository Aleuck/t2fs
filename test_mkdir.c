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
    DIR2 handleDir1 = mkdir2("Dir1");
    DIR2 handleDir2 = mkdir2("Dir2");
    DIR2 handleDir3 = mkdir2("Dir2");
    if (handleDir3 >= 0)
        printf("Erro não identificado! Nao deve permitir diretorios iguais\n");

    handleDir3 = mkdir2("Nomeinválido");
    if (handleDir3 >= 0)
        printf("Erro não identificado! Nao deve permitir nome invalido\n");

    handleDir3 = mkdir2("Nomeinvalidopqehmuitograndedemaistipoassimgrandedemaisbagarai");
    if (handleDir3 >= 0)
        printf("Erro não identificado! Nao deve permitir nome muito grande\n");

    handleDir3 = mkdir2("./././Dir3");
    if (handleDir3 < 0)
        printf("Erro não identificado! Deve permitir nome de arquivo com barras?\n");

    DIR2 handle = opendir2("/");
    ls(handle);
    return 0;
}