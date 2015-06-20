#include <stdio.h>
#include <stdint-gcc.h>
#include <string.h>
#include "../include/t2fs.h"
#include "../include/apidisk.h"


struct t2fs_superbloco superBloco;
int read_block(int id_block, char* buffer);
int write_block(int id_block, char* buffer);

/**
 * Retorna a quantidade de setores que formam um bloco, de acordo com informações
 * do superBloco.
 * Considera que tamanho do bloco sempre é um fator de SECTOR_SIZE
 */
int get_sectors_per_block()
{
    return superBloco.BlockSize / SECTOR_SIZE;
}

void print_sector(int id_block)
{
    char buffer[superBloco.BlockSize];
    read_block(id_block, buffer);

    int i;
    for (i = 0; i <= superBloco.BlockSize; i += 4){
        if (i%16 == 0)
            printf("\n");
        printf("%02xh ", (BYTE) buffer[i]);
        printf("%02xh ", (BYTE) buffer[i+1]);
        printf("%02xh ", (BYTE) buffer[i+2]);
        printf("%02xh ", (BYTE) buffer[i+3]);
    }
    printf("\n");
}


/**
 *  Funcao que escreve um bloco no disco. Buffer deve ter obrigatoriamente
 *  o tamanho de BLOCK_SIZE.
 *  Retorna 0 caso não ocorrer nenhum erro, -1 caso contrário.
 */
// TODO: Fazer testes
int write_block(int id_block, char* buffer)
{
    if (id_block >= superBloco.NofBlocks) {
        printf("ERRO: Tentando escrever no bloco %d", id_block);
        return -1;
    }

    int sectors_per_block = get_sectors_per_block();
    int start_sector = id_block * sectors_per_block;

    int sector_I;
    for (sector_I = 0; sector_I < sectors_per_block; sector_I++){
        // lê direto para o buffer na posição correta
        write_sector(start_sector + sector_I * sectors_per_block, buffer + SECTOR_SIZE * sector_I);
    }

    return 0;
}


int read_block(int id_block, char* buffer)
{
    if (id_block >= superBloco.NofBlocks) {
        printf("ERRO: Tentando ler bloco %d", id_block);
        return -1;
    }

    int sectors_per_block = get_sectors_per_block();
    int start_sector = id_block * sectors_per_block;

    int sector_I;
    for (sector_I = 0; sector_I < sectors_per_block; sector_I++){
        // lê direto para o buffer na posição correta
        read_sector(start_sector + sector_I * sectors_per_block, buffer + SECTOR_SIZE * sector_I);
    }

    return 0;
}

/**
 * Funcao que retorna o estado de um bloco:
 * 1: Usado, 0: Livre, -1: Erro na funcao
 */
// TODO: Considera que todos os blocos podem ser representados em um bloco.
//       Ampliar isso para o caso generico de mais blocos.
//       Os printfs so servem para debug, podem ser retirados.
int get_block_state(unsigned int block)
{
    if (block >= superBloco.NofBlocks) {
        printf("get_block_state(..) com bloco maior do que o numero de blocos do disco.\n");
        return -1;
    }

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

    char buffer[superBloco.BlockSize];
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

        char buffer_super_bloco[SECTOR_SIZE];
        read_sector(0, buffer_super_bloco);

        strncpy( superBloco.Id, buffer_super_bloco, 4);

        superBloco.Version =        (BYTE) buffer_super_bloco[4] + (buffer_super_bloco[5] << 8);
        superBloco.SuperBlockSize = (BYTE) buffer_super_bloco[6] + ((BYTE) buffer_super_bloco[7] << 8);
        superBloco.DiskSize =       (BYTE) buffer_super_bloco[8] + ((BYTE) buffer_super_bloco[9] << 8) + ((BYTE) buffer_super_bloco[10] << 16) + ((BYTE) buffer_super_bloco[11] << 24);
        superBloco.NofBlocks =      (BYTE) buffer_super_bloco[12] + ((BYTE) buffer_super_bloco[13] << 8) + ((BYTE) buffer_super_bloco[14] << 16) + ((BYTE) buffer_super_bloco[15] << 24);
        superBloco.BlockSize =      (BYTE) buffer_super_bloco[16] + ((BYTE) buffer_super_bloco[17] << 8) + ((BYTE) buffer_super_bloco[18] << 16) + ((BYTE) buffer_super_bloco[19] << 24);
        superBloco.BitmapBlocks =   (BYTE) buffer_super_bloco[20] + ((BYTE) buffer_super_bloco[21] << 8) + ((BYTE) buffer_super_bloco[22] << 16) + ((BYTE) buffer_super_bloco[23] << 24);
        superBloco.BitmapInodes =   (BYTE) buffer_super_bloco[24] + ((BYTE) buffer_super_bloco[25] << 8) + ((BYTE) buffer_super_bloco[26] << 16) + ((BYTE) buffer_super_bloco[27] << 24);
        superBloco.InodeBlock =     (BYTE) buffer_super_bloco[28] + ((BYTE) buffer_super_bloco[29] << 8) + ((BYTE) buffer_super_bloco[30] << 16) + ((BYTE) buffer_super_bloco[31] << 24);
        superBloco.FirstDataBlock = (BYTE) buffer_super_bloco[32] + ((BYTE) buffer_super_bloco[33] << 8) + ((BYTE) buffer_super_bloco[34] << 16) + ((BYTE) buffer_super_bloco[35] << 24);
    }
}

void printSuperBloco() {
    checkSuperBloco();

    printf("Id: %4.4s\n", superBloco.Id);
    printf("Version: Ano: %u Semestre: %u\n", superBloco.Version >> 4, superBloco.Version & ((DWORD)15));
    printf("SuperBlockSize: %u\n", superBloco.SuperBlockSize);
    printf("DiskSize: %u\n", superBloco.DiskSize);
    printf("NofBlocks: %u\n", superBloco.NofBlocks);
    printf("BlockSize: %u\n", superBloco.BlockSize);
    printf("BitmapBlockS: %u\n", superBloco.BitmapBlocks);
    printf("BitmapInodeS: %u\n", superBloco.BitmapInodes);
    printf("I-nodeBlock: %u\n", superBloco.InodeBlock);
    printf("FirstDataBlock: %u\n", superBloco.FirstDataBlock);

    // O tamanho de blocos utilizados pelo I-Node é:
    // (tamanho I-Node (64bytes) * numero de blocos) / tamanho do bloco
    // no arquivo disk_4_2048_2048 o tamanho do bloco de I-Nodes é 128, adicione 4 deslocamentos, e então o início
    // do bloco de dados é no INDEX 123 (inicia em 0)
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

    printf("bit = %d\n", get_block_state(1));
    print_sector(1);
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
