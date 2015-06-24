//
// Created by gianei on 22/06/2015.
//
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <stdint-gcc.h>
#include <assert.h>
#include "../include/filepath_operations.h"
#include "../include/block_io.h"


int get_inode_in_records(char *striped_file_name, struct t2fs_record *records, struct t2fs_inode *found_inode)
{
    int record_index = 0;
    int found = 0;
    while (record_index < get_records_in_block()){
        if (!strcmp(records[record_index].name, striped_file_name)){
            found = 1;
            break;
        }
        record_index++;
    }

    if (found){
        *found_inode = read_i_node(records[record_index].i_node);
        return 1;
    } else {
        return 0;
    }
}

int find_directory_in_inode(struct t2fs_superbloco superBloco, char *striped_file_name, struct t2fs_inode inode_atual, struct t2fs_inode *found_inode)
{
    int found = 0;
    int block_position = 0;
    struct t2fs_record records[get_records_in_block()];

    // le tamanho de blocos
    read_records(inode_atual.dataPtr[block_position], records);
    int blocks_file_size = records[0].blocksFileSize;

    while (!found && block_position < blocks_file_size){ //record[0] é o diretório . (atual)
        if (block_position < 10){ // nos ponteiros diretos
            read_records(inode_atual.dataPtr[block_position], records);
        } else if (block_position < (superBloco.BlockSize / 4) + 10){ //4 bytes por endereço
            //uma indireção
            char buffer[superBloco.BlockSize];
            read_block(inode_atual.singleIndPtr, buffer, superBloco);
            int block_index = (block_position - 10) * 4;
            int block_to_read = (BYTE) buffer[block_index] + ((BYTE) buffer[block_index+1] << 8) +
                                ((BYTE) buffer[block_index +2] << 16) + ((BYTE) buffer[block_index+3] << 24);
            read_records(block_to_read, records);
        } else{ //em indireção dupla
            //TODO conferir essa bizarrice
            char buffer[superBloco.BlockSize];
            read_block(inode_atual.doubleIndPtr, buffer, superBloco);
            int block_index = (block_position - 10 - ((superBloco.BlockSize / 4) + 10)) * 4;
            int block_to_read = (BYTE) buffer[block_index] + ((BYTE) buffer[block_index+1] << 8) +
                                ((BYTE) buffer[block_index +2] << 16) + ((BYTE) buffer[block_index+3] << 24);

            read_block(block_to_read, buffer, superBloco);

            //segunda indireção
            block_index = (block_position - 10 - ((superBloco.BlockSize / 4) + 10) - block_index*(superBloco.BlockSize / 4)) * 4;
            block_to_read = (BYTE) buffer[block_index] + ((BYTE) buffer[block_index+1] << 8) +
                            ((BYTE) buffer[block_index +2] << 16) + ((BYTE) buffer[block_index+3] << 24);

            read_records(block_to_read, records);
        }

        if (get_inode_in_records(striped_file_name, records, &inode_atual)){
            *found_inode = inode_atual;
            return 1;
        }

        block_position ++;
    }

    return 0;
}

char **str_split(char* a_str, const char a_delim) {
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

int find_inode_from_path(struct t2fs_superbloco superBloco, char *filename, struct t2fs_inode current_dir, struct t2fs_inode *found_inode)
{
    struct t2fs_inode current_inode = current_dir; // set starting position;
    struct t2fs_inode return_inode;

    char** tokens;

    int not_found = 0;
    if (filename[0] == '/'){ //lógica de método raíz
        current_inode = read_i_node(0);
//        printf("filename: %s\n", filename);
        filename+= sizeof(char);

        if (filename[1] == '\0'){
            *found_inode = current_inode;
            return 1;
        }

    }
//    printf("diretoria de pesquisa: %s\n", filename);

    tokens = str_split(filename, '/');

    if (tokens)
    {
        int i;
        for (i = 0; *(tokens + i); i++)
        {
            char *dir = *(tokens + i);
//            printf("striped: %s\n", dir);
//            printf("%d\n",i);
            if (find_directory_in_inode(superBloco, dir, current_inode, &return_inode)) {
//                printf("po%d\n",i);
                current_inode = return_inode;
//                printf("--current inode first block index: %u\n", current_inode.dataPtr[0]);
            } else {
//                printf("not found%d\n",i);
                not_found = 1;
                //deixa passar tudo para dar free
            }
            free(dir);
        }
        free(tokens);
    }

    if (not_found) {
//        printf("diretorio invalido");
        return 0;
    }

//    printf("achou diretorio");

    *found_inode = current_inode;
    return 1;
}

size_t strlstchar(const char *str, const char ch)
{
    char *chptr = strrchr(str, ch);
    if (chptr == NULL)
        return 0;
    return chptr - str +1;
}

char * get_string_after_bar(char *search)
{
    char* result    = 0;
    size_t position = strlstchar (search, '/');
    result = malloc(sizeof(char*) * (strlen(search) - position));
    memcpy( result, search + position , (strlen(search) - position) );
    return result;
}

char * get_string_before_last_bar(char *search)
{
    char* result    = 0;
    size_t position = strlstchar (search, '/');
    result = malloc(sizeof(char*) * position);
    memcpy( result, search, position );
    return result;
}

int last_index_of(char ch, char *string)
{
    int last_index = -1;
    int i = 0;
    while (string[i] != '\0') {
        if (string[i] == ch) {
            last_index = i;
        }
        i++;
    }
    return last_index;
}