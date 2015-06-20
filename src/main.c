#include <stdio.h>
#include "../include/t2fs.h"
#include "../include/apidisk.h"

void ls(DIR2 dirHandle){
    DIRENT2 dirEntry;

    while(readdir2(dirHandle, &dirEntry) >= 0) {
        printf("Name: %s Type: %d Size: %u\n", dirEntry.name, dirEntry.fileType, dirEntry.fileSize);
    }
}

int main(int argc, char *argv[]) {

    ls(opendir2("/"));

    return 0;
}
