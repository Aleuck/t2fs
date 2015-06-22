//
// Created by gianei on 22/06/2015.
//

#ifndef CODE2_FILEPATH_OPERATIONS_H
#define CODE2_FILEPATH_OPERATIONS_H

#include "t2fs.h"
int get_inode_in_records(char *striped_file_name, struct t2fs_record *records, struct t2fs_inode *found_inode);
int find_directory_in_inode(struct t2fs_superbloco, char *striped_file_name, struct t2fs_inode inode_atual, struct t2fs_inode *found_inode);
char ** str_split(char* a_str, const char a_delim);
int find_inode_from_path(struct t2fs_superbloco, char *filename, struct t2fs_inode current_dir, struct t2fs_inode *found_inode);



#endif //CODE2_FILEPATH_OPERATIONS_H
