#include <stdio.h>
#include "include/t2fs.h"
#include "include/bitmap_operations.h"

void ls(DIR2 dirHandle)
{
    DIRENT2 dirEntry;

    while(readdir2(dirHandle, &dirEntry) >= 0) {
        printf("Name: %s Type: %d Size: %lu\n", dirEntry.name, dirEntry.fileType, dirEntry.fileSize);
    }
}

int main(int argc, char *argv[]) {
    DIR2 dir = opendir2("inexistente");

    create2("Arquivo1");
    if(opendir2("Arquivo1") >= 0){
        printf("Nao pode permitir abrir arquivos");
    }

    if (dir >= 0){
        printf("Erro nao identificado! Diretorio nao existente nao pode ser aberto\n");
    }

    dir = opendir2("Dir1");

    ls(dir);

    closedir2(dir);

    return 0;

}
