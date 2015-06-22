#include <stdio.h>
#include "../include/t2fs.h"
#include "../include/apidisk.h"

void ls(DIR2 dirHandle)
{
    DIRENT2 dirEntry;

    while(readdir2(dirHandle, &dirEntry) == 0) {
        printf("Name: %s Type: %d Size: %lu\n", dirEntry.name, dirEntry.fileType, dirEntry.fileSize);
    }
}

int main(int argc, char *argv[])
{
//    DIR2 handle = opendir2("/");
//    ls(handle);
//    // Qual a diferenca entra fechar um arquivo ou um diretorio??
//    close2(0);

    char dir[] = "/.././..\0";
    chdir2(dir);
    char teste[30];
    getcwd2(teste, 30);

    return 0;
}
