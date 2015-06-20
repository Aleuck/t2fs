#include "../include/block_io.h"
#include <stdio.h>


int get_sectors_per_block(struct t2fs_superbloco superBloco);

/**
 *  Funcao que escreve um bloco no disco. Buffer deve ter obrigatoriamente
 *  o tamanho de BLOCK_SIZE.
 *  Retorna 0 caso não ocorrer nenhum erro, -1 caso contrário.
 */
// TODO: Fazer testes
int write_block(int id_block, char* buffer, struct t2fs_superbloco superBloco)
{
    if (id_block >= superBloco.NofBlocks) {
        printf("ERRO: Tentando escrever no bloco %d", id_block);
        return -1;
    }

    int sectors_per_block = get_sectors_per_block(superBloco);
    int start_sector = id_block * sectors_per_block;

    int sector_I;
    for (sector_I = 0; sector_I < sectors_per_block; sector_I++){
        // lê direto para o buffer na posição correta
        write_sector(start_sector + sector_I * sectors_per_block, buffer + SECTOR_SIZE * sector_I);
    }

    return 0;
}


int read_block(int id_block, char* buffer, struct t2fs_superbloco superBloco)
{
    if (id_block >= superBloco.NofBlocks) {
        printf("ERRO: Tentando ler bloco %d", id_block);
        return -1;
    }

    int sectors_per_block = get_sectors_per_block(superBloco);
    int start_sector = id_block * sectors_per_block;

    int sector_I;
    for (sector_I = 0; sector_I < sectors_per_block; sector_I++){
        // lê direto para o buffer na posição correta
        read_sector(start_sector + sector_I * sectors_per_block, buffer + SECTOR_SIZE * sector_I);
    }

    return 0;
}

/**
 * Retorna a quantidade de setores que formam um bloco, de acordo com informações
 * do superBloco.
 * Considera que tamanho do bloco sempre é um fator de SECTOR_SIZE
 */
int get_sectors_per_block(struct t2fs_superbloco superBloco)
{
    return superBloco.BlockSize / SECTOR_SIZE;
}
