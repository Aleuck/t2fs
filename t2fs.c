#include <stdio.h>
#include <stdint-gcc.h>
#include <string.h>
#include "t2fs.h"
#include "apidisk.h"

struct t2fs_superbloco superBloco;

void checkSuperBloco() {
    if (superBloco.DiskSize == 0) {

        char bufferSuperBloco[SECTOR_SIZE];
        read_sector(0, bufferSuperBloco);
        strncpy( superBloco.Id, bufferSuperBloco, 4);
        superBloco.Version =        bufferSuperBloco[4] + (bufferSuperBloco[5] << 8);
        superBloco.SuperBlockSize = bufferSuperBloco[6] + (bufferSuperBloco[7] << 8);
        superBloco.DiskSize =       bufferSuperBloco[8] + (bufferSuperBloco[9] << 8) + (bufferSuperBloco[10] << 16) + (bufferSuperBloco[11] << 24);
        superBloco.NofBlocks =      bufferSuperBloco[12] + (bufferSuperBloco[13] << 8) + (bufferSuperBloco[14] << 16) + (bufferSuperBloco[15] << 24);
        superBloco.BlockSize =      bufferSuperBloco[16] + (bufferSuperBloco[17] << 8) + (bufferSuperBloco[18] << 16) + (bufferSuperBloco[19] << 24);
        superBloco.BitmapBlocks =   bufferSuperBloco[20] + (bufferSuperBloco[21] << 8) + (bufferSuperBloco[22] << 16) + (bufferSuperBloco[23] << 24);
        superBloco.BitmapInodes =   bufferSuperBloco[24] + (bufferSuperBloco[25] << 8) + (bufferSuperBloco[26] << 16) + (bufferSuperBloco[27] << 24);
        superBloco.InodeBlock =     bufferSuperBloco[28] + (bufferSuperBloco[29] << 8) + (bufferSuperBloco[30] << 16) + (bufferSuperBloco[31] << 24);
        superBloco.FirstDataBlock = bufferSuperBloco[32] + (bufferSuperBloco[33] << 8) + (bufferSuperBloco[34] << 16) + (bufferSuperBloco[35] << 24);

    }
}

void printSuperBloco() {
    checkSuperBloco();

    printf("Id: %4.4s\n", superBloco.Id);
    printf("Version: %u\n", superBloco.Version); //TODO, incorreto pq primeiro 3 bits formam ano, e ultimo bit o semestre
    printf("SuperBlockSize: %u\n", superBloco.SuperBlockSize);
    printf("DiskSize: %u\n", superBloco.DiskSize);
    printf("NofBlocks: %u\n", superBloco.NofBlocks);
    printf("BlockSize: %u\n", superBloco.BlockSize);
    printf("BitmapBlockS: %u\n", superBloco.BitmapBlocks);
    printf("BitmapInodeS: %u\n", superBloco.BitmapInodes);
    printf("I-nodeBlock: %u\n", superBloco.InodeBlock);
    printf("FirstDataBlock: %u\n", superBloco.FirstDataBlock); //TODO, valor errado quando usado disco "disk_4_2048_2048.dat"
}

int identify2(char *name, int size) {
    int i = 0;
    char ids[] = "Alexandre Leuck (...), Gianei Sebastiany (213502)"
        " e Leonardo Hahn (207684)";

    checkSuperBloco();
    printSuperBloco();

    //TODO: Catch possible errors
    while (ids[i] != '\0') {
        if (i < size) {
            name[i] = ids[i];
        }
        i++;
    }
    if (i > size) {
        printf("\n**Size of the buffer passed is smaller "
               "than the identification string**\n\n");
    }
    return 0;
}

FILE2 create2(char *filename) {
    checkSuperBloco();
    return 0;
}

int delete2(char *filename) {
    checkSuperBloco();
    return 0;
}

FILE2 open2(char *filename) {
    checkSuperBloco();
    return 0;
}

int close2(FILE2 handle) {
    checkSuperBloco();
    return 0;
}

int read2(FILE2 handle, char *buffer, int size) {
    checkSuperBloco();
    return 0;
}

int write2(FILE2 handle, char *buffer, int size) {
    checkSuperBloco();
    return 0;
}

int seek2(FILE2 handle, unsigned int offset) {
    checkSuperBloco();
    return 0;
}

int mkdir2(char *pathname) {
    checkSuperBloco();
    return 0;
}

int rmdir2(char *pathname) {
    checkSuperBloco();
    return 0;
}

DIR2 opendir2(char *pathname) {
    checkSuperBloco();
    return 0;
}

int readdir2(DIR2 handle, DIRENT2 *dentry) {
    checkSuperBloco();
    return 0;
}

int closedir2(DIR2 handle) {
    checkSuperBloco();
    return 0;
}

int chdir2(char *pathname) {
    checkSuperBloco();
    return 0;
}

int getcwd2(char *pathname, int size) {
    checkSuperBloco();
    return 0;
}
