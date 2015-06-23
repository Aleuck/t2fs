#ifndef __BITMAP_OPERATIONS__
#define __BITMAP_OPERATIONS__

#include "t2fs.h"

typedef enum {BLOCK, INODE} bitmap_type;

int get_bitmap_state(unsigned int id_bit, bitmap_type type, struct t2fs_superbloco);
int set_on_bitmap(unsigned int id_block, short int bit_state, bitmap_type type, struct t2fs_superbloco);
void print_bitmap(bitmap_type type, struct t2fs_superbloco);
int get_free_bit_on_bitmap(bitmap_type type, struct t2fs_superbloco);

#endif
