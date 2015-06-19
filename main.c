#include <stdio.h>
#include "t2fs.h"
#include "apidisk.h"

int main(int argc, char *argv[]) {
    char name[100];
    int i = 0;

    identify2(name,80);

    while (name[i] != '\0') {
        printf("%c", name[i]);
        i++;
    }
    return 0;
}
