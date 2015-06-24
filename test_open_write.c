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
    FILE2 file = open2("inexistente");

    if (file >= 0){
        printf("Erro nao identificado! Arquivo nao existente nao pode ser aberto\n");
    }

    //Arquivo 1 deve ter sido criado previamente
    file = open2("Arquivo1");

    if (file >= 0){

        char buffer[] = "123456789123456789";
        if (write2(file, buffer, 18) < 0){
            printf("Erro ao escrever dados!\n");
        }

    } else {
        printf("Erro! Nao conseguiu abrir arquivo\n");
    }

    print_bitmap(BLOCK, get_superbloco());

    return 0;

}
