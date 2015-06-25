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

    if (dir >= 0){
        printf("Erro nao identificado! Diretorio nao existente nao pode ser aberto\n");
    }

//    mkdir2("Dir1");

    //Dir 1 deve ter sido criado previamente
    dir = opendir2("Dir1");

    //TODO criar diretórios indefinidamente
    ls(dir);

    closedir2(dir);

//    print_bitmap(BLOCK, get_superbloco());

    return 0;

}
