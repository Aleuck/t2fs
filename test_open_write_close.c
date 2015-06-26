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

//    if (open2("dir") >= 0){
//        printf("Nao pode permitir abrir diretorios");
//    }
    file = open2("Arquivo1");

    if (file >= 0){
        //TODO gravar indefinidamente até encher disco
        char buffer[] = "123456789123456789";
        if (write2(file, buffer, 18) < 0){
            printf("Erro ao escrever dados!\n");
        }

    } else {
        printf("Erro! Nao conseguiu abrir arquivo\n");
    }

    close2(file);
    char buffer[] = "5555";
    if (write2(file, buffer, 4) >= 0){
        printf("Erro! Escreveu mesmo depois de fechado!\n");
    }

    return 0;

}
