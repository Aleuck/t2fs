#ifndef __BLOCK_IO__
#define __BLOCK_IO__

#include "apidisk.h"
#include "t2fs.h"

int write_block(int id_block, char* buffer, struct t2fs_superbloco);
int read_block(int id_block, char* buffer, struct t2fs_superbloco);

#endif
