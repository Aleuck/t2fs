#include <stdio.h>
#include "./include/t2fs.h"

void ls(DIR2 dirHandle)
{
    DIRENT2 dirEntry;

    while(readdir2(dirHandle, &dirEntry) >= 0) {
        printf("Name: %s Type: %d Size: %lu\n", dirEntry.name, dirEntry.fileType, dirEntry.fileSize);
    }
}

void print_actual_path() {
    char actualPath[250];
    getcwd2(actualPath, 250);
    printf("Diretorio atual: %s\n",actualPath);
    ls(opendir2(actualPath));
}

int main(int argc, char *argv[]) {
    print_actual_path();

    DIR2 dir1 = mkdir2("dir1");

    chdir2("dir1");
    print_actual_path();

    DIR2 dir2 = mkdir2("dir2");

    chdir2("dir2");
    print_actual_path();

    DIR2 dir3 = mkdir2("dir3");
    DIR2 dir4 = mkdir2("dir4");

    print_actual_path();

    chdir2("../..");
    print_actual_path();
    chdir2("dir1/dir2/dir4");
    print_actual_path();

    chdir2("/dir1");
    print_actual_path();


    chdir2("/");
    int count;
    char str[16];
    for (count = 0; count < 50; count++){
        sprintf(str, "%d", count);
        mkdir2(str);
        chdir2(str);
    }
    print_actual_path();
}