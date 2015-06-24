#include <stdio.h>
#include "./include/t2fs.h"

int main(int argc, char *argv[]) {
    char identify[30];

    if (0 != identify2(identify, 20)){
        printf("Erro identificado: %s\n",identify);
    } else {
        printf("Erro nao identificado!\n");
    }

    if (0 != identify2(identify, 30)){
        printf("Erro identificado: %s\n",identify);
    } else {
        printf("Erro nao identificado!\n");
    }

    char identify_2[130];
    if (0 == identify2(identify_2, 130)){
        printf("%s\n",identify_2);
    } else {
        printf("Erro nao identificado!\n");
    }

    return 1;
}
