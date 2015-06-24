#include <stdio.h>
#include "./include/t2fs.h"

void ls(DIR2 dirHandle)
{
    DIRENT2 dirEntry;

    while(readdir2(dirHandle, &dirEntry) >= 0) {
        printf("Name: %s Type: %d Size: %lu\n", dirEntry.name, dirEntry.fileType, dirEntry.fileSize);
    }
}

int main(int argc, char *argv[])
{
    int handleArq = create2("Teste");
    //printf("%d", handleArq);
    char buffer[] = "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
        "qqqqqqqqqqqwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
        "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp"
        "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp"
        "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp"
        "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp"
        "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp"
        "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppoxxxxxxxxaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaz";
    int size = 0;
    while (buffer[size] != '\0') {
        size++;
    }
    printf("BUFFER SIZE: %d\n", size);
    write2(handleArq, buffer, 1000);

//    create2("MeuArquivo");
//    create2("LeoArq\0");
//    create2("HAHAHA");
//
//    printf("\n\n");
//    DIR2 handle = opendir2("/");
//ls(handle);
//    create2("MeuArquivo");
//    // Qual a diferenca entra fechar um arquivo ou um diretorio??
//    close2(0);

    /* char dir[] = "/.././..\0"; */
    /* chdir2(dir); */
    /* char teste[30]; */
    /* getcwd2(teste, 30); */

//    delete2("bla");
//    char teste[] = "es/as";
//    char *striped = get_string_after_bar(teste);
//    printf("striped: %s\n",striped);
//    striped = get_string_before_last_bar(teste);
//    printf("stripedB: %s\n",striped);
//    printf ("s%d",*striped);
//    mkdir2("AOSDJ");
    return 0;
}
