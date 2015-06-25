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
    chdir2("/");
    DIR2 handler = mkdir2("/bla");
    chdir2("/blah");
    //DIR2 handler2 = mkdir2("/bla/blah");
    DIR2 root = opendir2("/");
    //mkdir2("bla/blah");
    return 0;
}
