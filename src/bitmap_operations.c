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

        read_block(bitmap_block, buffer, superBloco);
    } else if (type == INODE) {
        DWORD bitmap_block = superBloco.BitmapInodes;
        printf("\nO bitmap de blocos esta localizado no bloco: %d\n", bitmap_block);
        printf("secao: %d\noffset: %d\n", section, offset);

        read_block(bitmap_block, buffer, superBloco);
    }

    bit = (buffer[section] >> (7 - offset)) & 1;

    return bit;
}

int set_on_bitmap(unsigned int id_block, short int bit_state, bitmap_type type, struct t2fs_superbloco superBloco)
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

    if (get_bitmap_state(id_block, type, superBloco) == bit_state) {
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
        printf("ISSO NUNCA DEVE SER IMPRESSO");
        return -1;
    }

    block_buffer[section] = new_section;

    if (type == BLOCK) {
        write_block(id_bitmap_block, block_buffer, superBloco);
    } else if (type == INODE) {
        read_block(id_bitmap_inode, block_buffer, superBloco);
    }

    return 0;
}
