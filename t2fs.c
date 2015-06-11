#include <stdio.h>
#include "t2fs.h"

/**
 * Função usada para identificar os desenvolvedores do T2FS. Essa função copia um string de identificação para o
 * ponteiro indicado por “name”. Essa cópia não pode exceder o tamanho do buffer, informado pelo parâmetro
 * “size”. O string deve ser formado apenas por caracteres ASCII (Valores entre 0x20 e 0x7A) e terminado por
 * ‘\0’ com o nome e número do cartão dos participantes do grupo.
 * Se a operação foi rea
 */
int identify2(char *name, int size) {
    //TODO
    printf("%s", "Gianei - 213502\0");
    return 0;
}

FILE2 create2(char *filename) {

    return 0;
}

int delete2(char *filename) {
    return 0;
}

FILE2 open2(char *filename) {
    return 0;
}

int close2(FILE2 handle) {
    return 0;
}

int read2(FILE2 handle, char *buffer, int size) {
    return 0;
}

int write2(FILE2 handle, char *buffer, int size) {
    return 0;
}

int seek2(FILE2 handle, unsigned int offset) {
    return 0;
}

int mkdir2(char *pathname) {
    return 0;
}

int rmdir2(char *pathname) {
    return 0;
}

DIR2 opendir2(char *pathname) {
    return 0;
}

int readdir2(DIR2 handle, DIRENT2 *dentry) {
    return 0;
}

int closedir2(DIR2 handle) {
    return 0;
}

int chdir2(char *pathname) {
    return 0;
}

int getcwd2(char *pathname, int size) {
    return 0;
}
