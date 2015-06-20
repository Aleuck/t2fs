#include <stdio.h>
#include "../include/t2fs.h"
#include "../include/apidisk.h"

int main(int argc, char *argv[]) {
    char name[100];
    int i = 0;

    identify2(name,80);

    while (name[i] != '\0') {
        printf("%c", name[i]);
        i++;
    }

//    chdir2("asd");
    return 0;
}
