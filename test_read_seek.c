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
    FILE2 file = create2("bigfile");
    int count;
    char str[9];
    str[9] = '\0';
    if (file >= 0) {
        for (count = 0; count < 50; count++) {
            sprintf(str, "%08d", count);
            write2(file, str, 8);
        }
    } else {
        file = open2("bigfile");
    }
    seek2(file, 0);
    read2(file, str, 8);
    printf("%s\n", str);
    read2(file, str, 8);
    printf("%s\n", str);
    read2(file, str, 8);
    printf("%s\n", str);
    seek2(file, 8*40);
    read2(file, str, 8);
    printf("Deve ser 39: %s\n", str);

    close2(file);

    return 0;
}
