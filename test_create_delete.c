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
    DIR2 handle = opendir2("/");
    printf("Raiz antes de criar\n");
    ls(handle);

    int handleArq1 = create2("Arquivo1");
    int handleArq2 = create2("Arquivo2");
    int handleArq3 = create2("Arquivo2");
    if (handleArq3 >= 0)
        printf("Erro n�o identificado! Nao deve permitir arquivos iguais\n");

    handleArq3 = create2("Nomeinv�lido");
    if (handleArq3 >= 0)
        printf("Erro n�o identificado! Nao deve permitir nome invalido\n");

    handleArq3 = create2("Nomeinvalidopqehmuitograndedemaistipoassimgrandedemaisbagarai");
    if (handleArq3 >= 0)
        printf("Erro n�o identificado! Nao deve permitir nome muito grande\n");

    handleArq3 = create2("./././arquivo3");
    if (handleArq3 >= 0)
        printf("Erro n�o identificado! N�o deve permitir nome de arquivo com barras?\n");

    printf("Raiz depois de criar\n");
    handle = opendir2("/"); //handle precisa ler de novo pra resetar o pointer dele
    ls(handle);

    if (delete2("Arquivo2") < 0){
        printf("Nao conseguiu apagar\n");
    }

    if (delete2("Arquivo1") < 0){
        printf("Nao conseguiu apagar\n");
    }

    if (delete2("Arquivo1") >= 0){
        printf("Apagou algo inexistente\n");
    }

    printf("Raiz depois de apagar\n");
    handle = opendir2("/");
    ls(handle);
    return 0;
}