#include <stdio.h>
#include <stdint-gcc.h>
#include <string.h>
#include "../include/t2fs.h"
#include "../include/apidisk.h"

#define BLOCK_SIZE (4 * SECTOR_SIZE)

struct t2fs_superbloco superBloco;

/* Funcao que le um bloco do disco. Buffer deve ter obrigatoriamente
 * o tamanho de BLOCK_SIZE
 */
void read_block(int idx, char* buffer)
{
    int sector1 = idx*4;
    int sector2 = sector1+1;
    int sector3 = sector2+1;
    int sector4 = sector3+1;
    char sector_buffer[SECTOR_SIZE];

    read_sector(sector1, sector_buffer);
    memcpy(buffer, sector_buffer, SECTOR_SIZE);
    read_sector(sector2, sector_buffer);
    memcpy(buffer+SECTOR_SIZE, sector_buffer, SECTOR_SIZE);
    read_sector(sector3, sector_buffer);
    memcpy(buffer+SECTOR_SIZE*2, sector_buffer, SECTOR_SIZE);
    read_sector(sector4, sector_buffer);
    memcpy(buffer+SECTOR_SIZE*3, sector_buffer, SECTOR_SIZE);
}

/**
 * Funcao que retorna o estado de um bloco:
 * 1: Usado, 0: Livre, -1: Erro na funcao
 */
// TODO: Considera que todos os blocos podem ser representados em um bloco.
//       Ampliar isso para o caso generico de mais blocos.
//       Os printfs so servem para debug, podem ser retirados.
int get_block_state(int block)
{
    // Bloco em que esta localizado o bitmap de blocos
    DWORD bitmap_block = superBloco.BitmapBlocks;
    printf("\nO bitmap de blocos esta localizado no bloco: %d\n", bitmap_block);
    // Cada char contem 8 bits, portanto dividimos o numero de bloco por 8.
    int bitmap_size = superBloco.NofBlocks / 8;
    printf("O tamanho do bitmap eh: %d\n", bitmap_size);

    int bit;
    int section = block / 8; // Que posicao i do buffer[i] o bloco se encontra.
    int offset  = block % 8; // Que deslocamento dentro dos 8 bits o bloco tem.
    printf("secao: %d\noffset: %d\n", section, offset);

    char buffer[BLOCK_SIZE];
    read_block(bitmap_block, buffer);

    bit = (buffer[section] >> (7 - offset)) & 1;

    return bit;
}

/* Nao sei se essa funcao tem alguma utilidade */
long unsigned int number_of_sectors()
{
    char buffer[SECTOR_SIZE];
    long unsigned int i = 0;
    long unsigned int sectors;
    while (read_sector(i, buffer) == 0) {
        i++;
    }
    sectors = i-1;

    return sectors;
}

void checkSuperBloco()
{
    if (superBloco.DiskSize == 0) {

        // Tamanho do Bloco e 4x o tamanho do Setor
        char bufferSuperBloco[BLOCK_SIZE];
        read_block(0, bufferSuperBloco);

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

    printf("bit = %d\n", get_block_state(72));
    return 0;
}

FILE2 create2(char *filename)
{
    checkSuperBloco();


    return 0;
}

int delete2(char *filename)
{
    checkSuperBloco();
    return 0;
}

FILE2 open2(char *filename)
{
    checkSuperBloco();
    return 0;
}

int close2(FILE2 handle)
{
    checkSuperBloco();
    return 0;
}

int read2(FILE2 handle, char *buffer, int size)
{
    checkSuperBloco();
    return 0;
}

int write2(FILE2 handle, char *buffer, int size)
{
    checkSuperBloco();
    return 0;
}

int seek2(FILE2 handle, unsigned int offset)
{
    checkSuperBloco();
    return 0;
}

int mkdir2(char *pathname)
{
    checkSuperBloco();
    return 0;
}

int rmdir2(char *pathname)
{
    checkSuperBloco();
    return 0;
}

DIR2 opendir2(char *pathname)
{
    checkSuperBloco();
    return 0;
}

int readdir2(DIR2 handle, DIRENT2 *dentry)
{
    checkSuperBloco();
    return 0;
}

int closedir2(DIR2 handle)
{
    checkSuperBloco();
    return 0;
}

int chdir2(char *pathname)
{
    checkSuperBloco();
    return 0;
}

int getcwd2(char *pathname, int size)
{
    checkSuperBloco();
    return 0;
}
