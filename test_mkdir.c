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
    printf("\nmkdir com path absoluto:\n");
    printf("mkdir2: /home\n");
    mkdir2("/home");
    printf("mkdir2: /home/usuario\n");
    mkdir2("/home/usuario");

    printf("\nls: /\n");
    DIR2 root = opendir2("/");
    ls(root);
    closedir2(root);

    printf("\nls: /home\n");
    DIR2 home = opendir2("/home");
    ls(home);
    closedir2(home);

    printf("\nmkdir2 com path relativo a raiz:\n");
    printf("mkdir2: bin\n");
    mkdir2("bin");
    printf("mkdir2: home/usuario2\n");
    mkdir2("home/usuario2");

    printf("\nls: /\n");
    root = opendir2("/");
    ls(root);
    closedir2(root);

    printf("\nls: /home\n");
    home = opendir2("/home");
    ls(home);
    closedir2(home);

    printf("\nchdir2: /home\n");
    chdir2("/home");

    printf("mkdir2: usuario3\n");
    mkdir2("usuario3");

    printf("\nls: /home\n");
    home = opendir2("/home");
    ls(home);
    closedir2(home);

    char name[30];
    int i;
    printf("criando 10000 diretorios em /home\n");
    for (i = 0; i < 10000; i++) {
        sprintf(name, "u%d", i);
        mkdir2(name);
    }

    printf("\nls: /home\n");
    home = opendir2("/home");
    ls(home);
    closedir2(home);
}
