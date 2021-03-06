#include "../include/bitmap_operations.h"
#include "../include/block_io.h"
#include <stdio.h>

/**
 * Funcao que retorna o estado de um bloco:
 * 1: Usado, 0: Livre, -1: Erro na funcao
 */
// TODO: Considera que todos os blocos podem ser representados em um bloco.
//       Ampliar isso para o caso generico de mais blocos.
//       Os printfs so servem para debug, podem ser retirados.
int get_bitmap_state(unsigned int id_bit, bitmap_type type, struct t2fs_superbloco superBloco)
{
    if (type == BLOCK && id_bit >= superBloco.NofBlocks) {
        printf("get_bitmap_state(..) com bloco maior do que o numero de blocos do disco: %d.\n",id_bit);
        return -1;
    } else if (type == INODE && id_bit >= (superBloco.BlockSize * (superBloco.FirstDataBlock - superBloco.InodeBlock)) / 64) {
        printf("git_bitmap_state(..) com inode maior do que o numero de inodes no disco.\n");
        return -1;
    }

    int bit;
    int section = id_bit >> 3; // Que posicao i do buffer[i] o bloco se encontra.
    int offset  = 7 - (id_bit & 7); // Que deslocamento dentro dos 8 bits o bloco tem.
    char buffer[superBloco.BlockSize];

    if (type == BLOCK) {
        DWORD bitmap_block = superBloco.BitmapBlocks;

        //printf("\nO bitmap de blocos esta localizado no bloco: %d\n", bitmap_block);
        //printf("secao: %d\noffset: %d\n", section, offset);

        read_block(bitmap_block, buffer, superBloco);
    } else if (type == INODE) {
        DWORD bitmap_block = superBloco.BitmapInodes;
        //printf("\nO bitmap de inodes esta localizado no bloco: %d\n", bitmap_block);
        //printf("secao: %d\noffset: %d\n", section, offset);

        read_block(bitmap_block, buffer, superBloco);
    }

    bit = (buffer[section] >> (7 - offset)) & 1;

    return bit;
}

void print_bitmap(bitmap_type type, struct t2fs_superbloco superBloco)
{
    unsigned int i;
    for (i = 0; i < superBloco.NofBlocks; i++) {
        printf("%d = %d ", i, get_bitmap_state(i, type, superBloco));
    }
}

int set_on_bitmap(unsigned int id_bit, short int bit_state, bitmap_type type, struct t2fs_superbloco superBloco)
{
    if (type == BLOCK && id_bit >= superBloco.NofBlocks) {
        printf("Tentando setar o bloco %d que nao existe.\n", id_bit);
        return -1;
    } else if (type == INODE && id_bit >= (superBloco.BlockSize * (superBloco.FirstDataBlock - superBloco.InodeBlock)) / 64) {
        printf("Tentando setar o inode %d que nao existe.\n", id_bit);
        return -1;
    } else if (bit_state != 0 && bit_state != 1) {
        printf("Valor passado de bit invalido.\n");
        return -1;
    }

    if (get_bitmap_state(id_bit, type, superBloco) == bit_state) {
        return 0;
    }

    int id_bitmap_block = superBloco.BitmapBlocks;
    int id_bitmap_inode = superBloco.BitmapInodes;
    char block_buffer[superBloco.BlockSize];

    int section = id_bit >> 3;
    int offset  = 7 - (id_bit & 7);

    char new_section;

    if (type == BLOCK) {
        read_block(id_bitmap_block, block_buffer, superBloco);
    } else if (type == INODE) {
        read_block(id_bitmap_inode, block_buffer, superBloco);
    }

    switch (bit_state) {
    case 0:
        new_section = block_buffer[section] - (1 << (7 - offset));
        break;
    case 1:
        new_section = block_buffer[section] + (1 << (7 - offset));
        break;
    default:
        printf("\nISSO NUNCA DEVE SER IMPRESSO\n");
        return -1;
    }

    block_buffer[section] = new_section;

    if (type == BLOCK) {
        write_block(id_bitmap_block, block_buffer, superBloco);
    } else if (type == INODE) {
        write_block(id_bitmap_inode, block_buffer, superBloco);
    }

    return 0;
}

/**
 *  Funcao que retorna o indice do primeiro elemento livre no
 *  bitmap indicado por *type*.
 */
int get_free_bit_on_bitmap(bitmap_type type, struct t2fs_superbloco superBloco)
{
    int idx = 0;
    while (get_bitmap_state(idx, type, superBloco) != 0) {
        idx++;
        if (type == BLOCK && idx >= superBloco.NofBlocks) {
            return -1;
        } else if (type == INODE && idx >= superBloco.NofBlocks) {
            return -1;
        }
    }
    return idx;
}
