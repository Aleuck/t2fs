#include <stdio.h>
#include <stdint-gcc.h>
#include <string.h>
#include "../include/t2fs.h"
#include "../include/apidisk.h"

#define INODE_SIZE 64
#define RECORD_SIZE 64

typedef enum {BLOCK, INODE} bitmap_type;

struct t2fs_superbloco superBloco;

/* Prototipo de Funcoes */
int read_block(int id_block, char* buffer);
int write_block(int id_block, char* buffer);
struct t2fs_inode read_i_node(int id_inode);
void checkSuperBloco();
int get_bitmap_state(unsigned int id_bit, bitmap_type type);

/***********************************/
/* Definicao do corpo das Funcoes **/
/***********************************/

int set_on_bitmap(unsigned int id_block, short int bit_state, bitmap_type type)
{
    if (type == BLOCK && id_block >= superBloco.NofBlocks) {
        printf("Tentando setar o bloco %d que nao existe.", id_block);
        return -1;
    } else if (type == INODE && 0 /*TODO: Verificar numero de inodes */) {
        printf("Tentando setar o inode %d que nao existe.", id_block);
        return -1;
    } else if (bit_state != 0 && bit_state != 1) {
        printf("Valor passado de bit invalido.\n");
        return -1;
    }


    if (get_bitmap_state(id_block, type) == bit_state) {
        // Estado do bit estava igual ao pedido
        return 0;
    }

    int id_bitmap_block = superBloco.BitmapBlocks;
    int id_bitmap_inode = superBloco.BitmapInodes;
    char block_buffer[superBloco.BlockSize];

    int section = id_block / 8;
    int offset  = id_block % 8;

    char new_section;

    if (type == BLOCK) {
        read_block(id_bitmap_block, block_buffer);
    } else if (type == INODE) {
        read_block(id_bitmap_inode, block_buffer);
    }

    switch (bit_state) {
    case 0:
        new_section = block_buffer[section] - (1 << (7 - offset));
        break;
    case 1:
        new_section = block_buffer[section] + (1 << (7 - offset));
        break;
    default:
        printf("ISSO NUNCA DEVE SER IMPRESSO");
        return -1;
    }

    block_buffer[section] = new_section;

    if (type == BLOCK) {
        write_block(id_bitmap_block, block_buffer);
    } else if (type == INODE) {
        read_block(id_bitmap_inode, block_buffer);
    }

    return 0;
}

void print_record(struct t2fs_record record) {
    printf("Tipo: ");
    switch (record.TypeVal){
        case 0:
            printf("Invalido\n");
            break;
        case 1:
            printf("Arquivo regular\n");
            break;
        case 2:
            printf("Arquivo diretorio\n");
            break;
        default:
            printf("ERRO\n");
            break;
    }

    printf("Nome: %s\n", record.name);

    printf("Blocks: %u\n", record.blocksFileSize);
    printf("Size: %u\n", record.bytesFileSize);
    printf("I-Node Id: %u\n", record.i_node);
}

int get_records_in_block() { return superBloco.BlockSize / RECORD_SIZE; }

/**
 * le estruturas a partir de um bloco com dados de diretório (apontado por um i-node)
 * 
 */
struct t2fs_record read_record(DWORD id_block) {
    struct t2fs_record records[get_records_in_block()]; //sempre há X records por bloco

    char buffer[superBloco.BlockSize];
    read_block(id_block, buffer);

    int r;
    for (r = 0; r < get_records_in_block(); r++){
        records[r].TypeVal = (BYTE) buffer[r * RECORD_SIZE];

        memcpy( records[r].name, &buffer[r * RECORD_SIZE +1], 31);
        records[r].name[31] = '\0'; //força fim de string


        records[r].blocksFileSize =  (BYTE) buffer[r * RECORD_SIZE + 32] + ((BYTE) buffer[r * RECORD_SIZE + 33] << 8) + ((BYTE) buffer[r * RECORD_SIZE + 34] << 16) + ((BYTE) buffer[r * RECORD_SIZE + 35] << 24);
        records[r].bytesFileSize =   (BYTE) buffer[r * RECORD_SIZE + 36] + ((BYTE) buffer[r * RECORD_SIZE + 37] << 8) + ((BYTE) buffer[r * RECORD_SIZE + 38] << 16) + ((BYTE) buffer[r * RECORD_SIZE + 39] << 24);
        records[r].i_node =          (BYTE) buffer[r * RECORD_SIZE + 40] + ((BYTE) buffer[r * RECORD_SIZE + 41] << 8) + ((BYTE) buffer[r * RECORD_SIZE + 42] << 16) + ((BYTE) buffer[r * RECORD_SIZE + 43] << 24);

    }


    return records[0];
}


void print_inode(struct t2fs_inode inode) {
    int i = 0;

    printf("Ponteiros diretos:\n");
    for (i = 0; i < 10; i++){
        printf("%08xh; ", inode.dataPtr[i]);
    }
    printf("\n");

    printf("Indirecao simples: 0x%08x\n", inode.singleIndPtr);
    printf("Indirecao dupla: 0x%08x\n", inode.doubleIndPtr);

}

struct t2fs_inode read_i_node(int id_inode)
{
    struct t2fs_inode inode;

    int block_relative = ((id_inode) * INODE_SIZE) / superBloco.BlockSize;
//    printf("block relative: %d\n", block_relative);
    int inode_relative = (((id_inode) * INODE_SIZE) % superBloco.BlockSize);
//    printf("inode relative: %d\n", inode_relative);

    //le bloco do inodes
    char buffer[superBloco.BlockSize];
    read_block(superBloco.InodeBlock + block_relative, buffer);

    //le
    int i, j;
    for (i = 0, j = 0; i < 40; i+=4, j++){
        inode.dataPtr[j] = (BYTE) buffer[inode_relative + i] + ((BYTE) buffer[inode_relative + i+1] << 8) +
                           ((BYTE) buffer[inode_relative + i+2] << 16) + ((BYTE) buffer[inode_relative + i+3] << 24);
    }

    //indireção simples
    i = 40;
    inode.singleIndPtr = (BYTE) buffer[inode_relative + i] + ((BYTE) buffer[inode_relative + i+1] << 8) +
                         ((BYTE) buffer[inode_relative + i+2] << 16) + ((BYTE) buffer[inode_relative + i+3] << 24);
    //indireção dupla
    i = 44;
    inode.doubleIndPtr = (BYTE) buffer[inode_relative + i] + ((BYTE) buffer[inode_relative + i+1] << 8) +
                         ((BYTE) buffer[inode_relative + i+2] << 16) + ((BYTE) buffer[inode_relative + i+3] << 24);

    return inode;
}

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
        printf("0x%02x ", (BYTE) buffer[i]);
        printf("0x%02x ", (BYTE) buffer[i+1]);
        printf("0x%02x ", (BYTE) buffer[i+2]);
        printf("0x%02x ", (BYTE) buffer[i+3]);
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
int get_bitmap_state(unsigned int id_bit, bitmap_type type)
{
    if (type == BLOCK && id_bit >= superBloco.NofBlocks) {
        printf("get_bitmap_state(..) com bloco maior do que o numero de blocos do disco.\n");
        return -1;
    } else if (type == INODE && 0 /*TODO: Checar o numero de inodes */) {
        printf("git_bitmap_state(..) com inode maior do que o numero de inodes no disco.\n");
        return -1;
    }

    int bit;
    int section = id_bit / 8; // Que posicao i do buffer[i] o bloco se encontra.
    int offset  = id_bit % 8; // Que deslocamento dentro dos 8 bits o bloco tem.
    char buffer[superBloco.BlockSize];

    if (type == BLOCK) {
        DWORD bitmap_block = superBloco.BitmapBlocks;

        printf("\nO bitmap de blocos esta localizado no bloco: %d\n", bitmap_block);
        printf("secao: %d\noffset: %d\n", section, offset);

        read_block(bitmap_block, buffer);
    } else if (type == INODE) {
        DWORD bitmap_block = superBloco.BitmapInodes;
        printf("\nO bitmap de blocos esta localizado no bloco: %d\n", bitmap_block);
        printf("secao: %d\noffset: %d\n", section, offset);

        read_block(bitmap_block, buffer);
    }

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
        " e Leonardo Hahn (207684)\n\0";

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

//    printf("bit = %d\n", get_block_state(1));
//    print_sector(3);
    struct t2fs_inode inode = read_i_node(0);
    print_inode(inode);

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

    struct t2fs_record tes = read_record(read_i_node(0).dataPtr[0]);
    return 0;
}

int getcwd2(char *pathname, int size)
{
    checkSuperBloco();
    return 0;
}

void checkSuperBloco()
{
    if (superBloco.DiskSize == 0) {

        char buffer_super_bloco[SECTOR_SIZE];
        read_sector(0, buffer_super_bloco);

        strncpy( superBloco.Id, buffer_super_bloco, 4);

        superBloco.Version =        (BYTE) buffer_super_bloco[4] + (buffer_super_bloco[5] << 8);

        superBloco.SuperBlockSize = (BYTE) buffer_super_bloco[6] + ((BYTE) buffer_super_bloco[7] << 8);

        superBloco.DiskSize =       (BYTE) buffer_super_bloco[8] + ((BYTE) buffer_super_bloco[9] << 8) +
            ((BYTE) buffer_super_bloco[10] << 16) + ((BYTE) buffer_super_bloco[11] << 24);

        superBloco.NofBlocks =      (BYTE) buffer_super_bloco[12] + ((BYTE) buffer_super_bloco[13] << 8) +
            ((BYTE) buffer_super_bloco[14] << 16) + ((BYTE) buffer_super_bloco[15] << 24);

        superBloco.BlockSize =      (BYTE) buffer_super_bloco[16] + ((BYTE) buffer_super_bloco[17] << 8) +
            ((BYTE) buffer_super_bloco[18] << 16) + ((BYTE) buffer_super_bloco[19] << 24);

        superBloco.BitmapBlocks =   (BYTE) buffer_super_bloco[20] + ((BYTE) buffer_super_bloco[21] << 8) +
            ((BYTE) buffer_super_bloco[22] << 16) + ((BYTE) buffer_super_bloco[23] << 24);

        superBloco.BitmapInodes =   (BYTE) buffer_super_bloco[24] + ((BYTE) buffer_super_bloco[25] << 8) +
            ((BYTE) buffer_super_bloco[26] << 16) + ((BYTE) buffer_super_bloco[27] << 24);

        superBloco.InodeBlock =     (BYTE) buffer_super_bloco[28] + ((BYTE) buffer_super_bloco[29] << 8) +
            ((BYTE) buffer_super_bloco[30] << 16) + ((BYTE) buffer_super_bloco[31] << 24);

        superBloco.FirstDataBlock = (BYTE) buffer_super_bloco[32] + ((BYTE) buffer_super_bloco[33] << 8) +
            ((BYTE) buffer_super_bloco[34] << 16) + ((BYTE) buffer_super_bloco[35] << 24);
    }
}
