#include <stdio.h>
#include <stdint-gcc.h>
#include <string.h>
#include <stdlib.h>
#include "../include/t2fs.h"
#include "../include/apidisk.h"
#include "../include/bitmap_operations.h"
#include "../include/block_io.h"
#include "../include/filepath_operations.h"

#define INODE_SIZE 64
#define RECORD_SIZE 64
#define MAX_OPEN_FILES 20
#define INVALID_POINTER 0x0FFFFFFFF

typedef enum {FILE_TYPE, DIR_TYPE} file_type;

typedef struct open_file {
    struct t2fs_record *record; // Adicona record a arquivo aberto
    struct t2fs_inode *inode;
    DWORD parent_inode;
    //unsigned int id_inode; // Remove, pois ja tem no *record
    unsigned int position;
    int handle;
    struct open_file *next;
} OPEN_FILE;

typedef struct {
    int size;
    OPEN_FILE *first;
} OPEN_FILES;

typedef struct path_node {
    struct t2fs_record record;
    struct path_node *previous;
} PATH_NODE;

typedef struct path {
    PATH_NODE *current;
} PATH;

struct t2fs_superbloco superBloco;

int current_handle = 0;       // Indica o valor do handle do prox. arquivo aberto
OPEN_FILES *open_files;       // Lista encadeada dos arquivos abertos
OPEN_FILES *open_directories; // Lista encadeada dos diretorios abertos
struct t2fs_inode current_dir;
PATH current_path = {0};

/* Prototipo de Funcoes */

void checkSuperBloco();
int allocate_block(bitmap_type type);
int free_block(int id, bitmap_type type);
void init_indices_block(int id_block);
void init_data_block(int id_block);
void init_records_block(int id_block);
int get_last_abstract_block_from_inode(struct t2fs_inode *inode);
int get_position_eof(OPEN_FILE *open_file);
void print_record(struct t2fs_record record);
OPEN_FILE* get_file_from_list(int handle, file_type type);
void print_indices(int id_block);
int get_block_id_from_inode(int relative_index, struct t2fs_inode *inode);

int copy_path(PATH *dest, const PATH *origin);

// frees path and set it to '/'
int chdir2_root(PATH *current_path);

// advaces one directory in path to dirname, accepts . (do nothing) or .. (go back 1 directory);
int chdir2_simple(PATH *current_path, char *dirname);

// chdir any path (pathname is trunkated)
int chdir2_generic (PATH *current_path, char *pathname);

int free_path(PATH *path);

/***********************************/
/* Definicao do corpo das Funcoes **/
/***********************************/

/**
 * Usado para testes
 */
struct t2fs_superbloco get_superbloco() {
    checkSuperBloco();
    return superBloco;
}

int get_num_indices_in_block()
{
    return superBloco.BlockSize / sizeof(DWORD);
}

void write_records_on_block(int id_block, struct t2fs_record *records)
{
    char buffer[superBloco.BlockSize];
    read_block(id_block, buffer, superBloco);

    int i;
    for (i = 0; i < get_records_in_block(); i++) {
        memcpy(buffer+(i*RECORD_SIZE), &records[i].TypeVal, 1);
        memcpy(buffer+(i*RECORD_SIZE)+1, &records[i].name, 31);
        memcpy(buffer+(i*RECORD_SIZE)+32, &records[i].blocksFileSize, 4);
        memcpy(buffer+(i*RECORD_SIZE)+36, &records[i].bytesFileSize, 4);
        memcpy(buffer+(i*RECORD_SIZE)+40, &records[i].i_node, 4);
        memcpy(buffer+(i*RECORD_SIZE)+44, &records[i].Reserved, 20);
    }
    write_block(id_block, buffer, superBloco);
}

// Procura por um record com nome igual a *recordname,
// se encontrar, retorna o indice no bloco e preenche *record com os respectivos dados
// se não encontrar, retorna -1 e define record->TypeVal = 0xFF
int find_record_in_block(int id_block, char *record_name, struct t2fs_record *file_record)
{
    int records_in_block = get_records_in_block();
    struct t2fs_record records[records_in_block];
    int record_index;

    read_records(id_block, records);
    for (record_index = 0; record_index < records_in_block; record_index++) {
        if (records[record_index].TypeVal != TYPEVAL_INVALIDO) {
            if (strcmp(records[record_index].name, record_name) == 0) {
                memcpy(file_record, &(records[record_index]), sizeof(struct t2fs_record));
                return record_index;
            }
        }
    }
    file_record->TypeVal = TYPEVAL_INVALIDO;
    return -1;
}

int find_record_in_index_array(DWORD *dataPtr, int dataPtrLength, char *record_name, struct t2fs_record *file_record)
{
    int dataPtr_index;
    DWORD id_block;

    for (dataPtr_index = 0; dataPtr_index < dataPtrLength; dataPtr_index++) {
        id_block = dataPtr[dataPtr_index];
        if (id_block == 0x0FFFFFFFF) {
            // TODO: parar de procurar aqui? (return -1)
            continue;
        }
        if (find_record_in_block(id_block, record_name, file_record) != -1) {
            return dataPtr_index;
        }
    }
    return -1;

}

// preenche a estrutura file_record com o record encontrado
// retorna -1 se record_name não é encontrado
// retorna indice abstrato do bloco, ex:
//    2 se for estiver no terceiro index em dir_inode.dataPtr
//    10 se for o primeiro indice no bloco de indireção simples
int find_record_in_inode(struct t2fs_inode dir_inode, char *record_name, struct t2fs_record *file_record)
{
    int indices_in_block = get_num_indices_in_block();
    DWORD singleInd[indices_in_block];
    DWORD doubleInd[indices_in_block];
    int record_index;
    int acumulated_index = 0;
    int i;

    record_index = find_record_in_index_array(dir_inode.dataPtr, 10, record_name, file_record);
    if (record_index != -1) {
        return record_index;
    }
    acumulated_index = 10;
    if (dir_inode.singleIndPtr != INVALID_POINTER) {
        read_block(dir_inode.singleIndPtr, (char*) singleInd, superBloco);
        record_index = find_record_in_index_array(singleInd, indices_in_block, record_name, file_record);
        if (record_index != -1) {
            return record_index + acumulated_index;
        }
    } else {
        return -1;
    }
    acumulated_index += indices_in_block;
    if (dir_inode.doubleIndPtr != INVALID_POINTER) {
        read_block(dir_inode.doubleIndPtr, (char*) doubleInd, superBloco);
        for (i = 0; i < indices_in_block; i++) {
            read_block(doubleInd[i], (char*) singleInd, superBloco);
            record_index = find_record_in_index_array(singleInd, indices_in_block, record_name, file_record);
            if (record_index != -1) {
                return record_index + acumulated_index;
            }
            acumulated_index += indices_in_block;
        }
    }
    return -1;
}


int add_record_to_index_array(DWORD *dataPtr, int dataPtrLength, struct t2fs_record file_record)
{
    int records_in_block = get_records_in_block();
    struct t2fs_record records[records_in_block];
    DWORD id_block;
    int dataPtr_index;
    int record_index;

    for (dataPtr_index = 0; dataPtr_index < dataPtrLength; dataPtr_index++) {
        id_block = dataPtr[dataPtr_index];
        if (id_block == INVALID_POINTER) {
            id_block = allocate_block(BLOCK);
            init_records_block(id_block);
            dataPtr[dataPtr_index] = id_block;
        }
        read_records(id_block, records);
        for (record_index = 0; record_index < records_in_block; record_index++) {
            if (records[record_index].TypeVal == TYPEVAL_INVALIDO) {
                // encontrada posição livre
                break;
            } else {
                if (strcmp(records[record_index].name, file_record.name) == 0) {
                    // encontrado registro com mesmo nome
                    break;
                }
            }
        }
        if (record_index != records_in_block) {
            // Índice livre encontrado
            break;
        }
    }
    if (record_index != records_in_block) {
        records[record_index] = file_record;
        write_records_on_block(id_block, records);
        return 0;
    }
    return -1;
}

int add_record_to_inode(struct t2fs_inode *dir_inode, struct t2fs_record file_record)
{
    int ptrs_in_block = get_num_indices_in_block();
    DWORD singleInd[ptrs_in_block];
    DWORD doubleInd[ptrs_in_block];
    int i;

    // tenta inserir nos ponteiros diretos
    if (add_record_to_index_array(dir_inode->dataPtr, 10, file_record) == 0) {
        return 0;
    }

    // tenta usar indireção simples
    if (dir_inode->singleIndPtr == INVALID_POINTER) {
        dir_inode->singleIndPtr = allocate_block(BLOCK);
        init_indices_block(dir_inode->singleIndPtr);
    }
    read_block(dir_inode->singleIndPtr, (char*) singleInd, superBloco);
    if (add_record_to_index_array(singleInd, ptrs_in_block, file_record) == 0) {
        write_block(dir_inode->singleIndPtr, (char*) singleInd, superBloco);
        return 0;
    }

    // tenta usar indireção dupla
    if (dir_inode->doubleIndPtr == INVALID_POINTER) {
        dir_inode->doubleIndPtr = allocate_block(BLOCK);
        init_indices_block(dir_inode->doubleIndPtr);
    }
    read_block(dir_inode->doubleIndPtr, (char*) doubleInd, superBloco);
    for (i = 0; i < ptrs_in_block; i++) {
        if (doubleInd[i] == INVALID_POINTER) {
            doubleInd[i] = allocate_block(BLOCK);
            init_indices_block(doubleInd[i]);
            write_block(dir_inode->doubleIndPtr, (char*) doubleInd, superBloco);
        }
        read_block(doubleInd[i], (char*) singleInd, superBloco);
        if (add_record_to_index_array(singleInd, ptrs_in_block, file_record) == 0) {
            write_block(doubleInd[i], (char*) singleInd, superBloco);
            return 0;
        }
    }
    return -1;
}

int replace_record_in_index_array(DWORD *dataPtr, int dataPtrLength, char *replaced_record, struct t2fs_record new_record)
{
    int records_in_block = get_records_in_block();
    struct t2fs_record records[records_in_block];
    DWORD id_block;
    int dataPtr_index;
    int record_index;

    for (dataPtr_index = 0; dataPtr_index < dataPtrLength; dataPtr_index++) {
        id_block = dataPtr[dataPtr_index];
        if (id_block == INVALID_POINTER) {
            id_block = allocate_block(BLOCK);
            init_records_block(id_block);
            dataPtr[dataPtr_index] = id_block;
        }
        read_records(id_block, records);
        for (record_index = 0; record_index < records_in_block; record_index++) {
            if (records[record_index].TypeVal == TYPEVAL_INVALIDO) {
                // encontrada posição livre
                return -1;
            } else {
                if (strcmp(records[record_index].name, replaced_record) == 0) {
                    // encontrado registro com mesmo nome
                    break;
                }
            }
        }
        if (record_index != records_in_block) {
            // Índice livre encontrado
            break;
        }
    }
    // if (record_index != records_in_block) {
    //     records[record_index] = file_record;
    //     write_records_on_block(id_block, records);
    //     return 0;
    // }
    return -1;
}

int replace_record_in_inode(struct t2fs_inode *dir_inode, char *replaced_record, struct t2fs_record new_record)
{
    if (replace_record_in_index_array(dir_inode->dataPtr, 10, replaced_record, new_record) == 0) {
        return 0;
    }
}

int pop_last_record_from_inode(struct t2fs_inode *dir_inode, struct t2fs_record *last_record)
{}

int remove_record_from_inode(struct t2fs_inode *dir_inode, char *name)
{
    int ptrs_in_block = get_num_indices_in_block();
    DWORD singleInd[ptrs_in_block];
    DWORD doubleInd[ptrs_in_block];
    int i;
    struct t2fs_record record_to_remove;
    struct t2fs_record last_record;

    if (find_record_in_inode(*dir_inode, name, &record_to_remove) == -1) {
        printf("remove_record_from_inode: `%s` não foi encontrado", name);
        return -1;
    }

    pop_last_record_from_inode(dir_inode, &last_record);

    if (strcmp(last_record.name, record_to_remove.name) == 0) {
        // record era o ultimo
        return 0;
    }

    return replace_record_in_inode(dir_inode, name, last_record);
}

void print_record(struct t2fs_record record)
{
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

int get_records_in_block()
{
    return superBloco.BlockSize / RECORD_SIZE;
}

/**
 * le estruturas a partir de um bloco com dados de diretório (apontado por um i-node)
 *
 */
void read_records(DWORD id_block, struct t2fs_record records[])
{
    char buffer[superBloco.BlockSize];
    read_block(id_block, buffer, superBloco);
    int num_records = get_records_in_block();
    int r;
    for (r = 0; r < num_records; r++) {
        records[r].TypeVal = (BYTE) buffer[r * RECORD_SIZE];

        memcpy( records[r].name, &buffer[r * RECORD_SIZE +1], 31);
        records[r].name[31] = '\0'; //força fim de string

        records[r].blocksFileSize =  (BYTE) buffer[r * RECORD_SIZE + 32] + ((BYTE) buffer[r * RECORD_SIZE + 33] << 8) +
            ((BYTE) buffer[r * RECORD_SIZE + 34] << 16) + ((BYTE) buffer[r * RECORD_SIZE + 35] << 24);
        records[r].bytesFileSize =   (BYTE) buffer[r * RECORD_SIZE + 36] + ((BYTE) buffer[r * RECORD_SIZE + 37] << 8) +
            ((BYTE) buffer[r * RECORD_SIZE + 38] << 16) + ((BYTE) buffer[r * RECORD_SIZE + 39] << 24);
        records[r].i_node =          (BYTE) buffer[r * RECORD_SIZE + 40] + ((BYTE) buffer[r * RECORD_SIZE + 41] << 8) +
            ((BYTE) buffer[r * RECORD_SIZE + 42] << 16) + ((BYTE) buffer[r * RECORD_SIZE + 43] << 24);

    }
    // Seta as entradas para invalidas caso nao sejam diretorios ou arquivos
    for (r = 0; r < num_records; r++) {
        if (records[r].TypeVal != TYPEVAL_DIRETORIO && records[r].TypeVal != TYPEVAL_REGULAR) {
            records[r].TypeVal = TYPEVAL_INVALIDO;
        }
    }
}

void print_inode(struct t2fs_inode inode)
{
    int i = 0;

    printf("Ponteiros diretos:\n");
    for (i = 0; i < 10; i++){
        //printf("%08xh; ", inode.dataPtr[i]);
        printf("%d; ", inode.dataPtr[i]);
    }
    printf("\n");

    //printf("Indirecao simples: 0x%08x\n", inode.singleIndPtr);
    printf("Indirecao simples: %d\n", inode.singleIndPtr);
    //printf("Indirecao dupla: 0x%08x\n", inode.doubleIndPtr);
    printf("Indirecao dupla: %d\n", inode.doubleIndPtr);

}

int write_inode(int id_inode, struct t2fs_inode *inode)
{
    int i;
    char buffer[superBloco.BlockSize];
    int block_relative = ((id_inode) * INODE_SIZE) / superBloco.BlockSize;
    int inode_relative = (((id_inode) * INODE_SIZE) % superBloco.BlockSize);

    if (id_inode >= superBloco.NofBlocks) {
        printf("Tentando escrever no inode %d que nao existe.\n", id_inode);
        return -1;
    }

    read_block(superBloco.InodeBlock + block_relative, buffer, superBloco);

    for (i = 0; i < 10; i++) {
        memcpy(buffer+inode_relative+(i*4), &inode->dataPtr[i], 4);
    }

    memcpy(buffer+inode_relative+40, &inode->singleIndPtr, 4);
    memcpy(buffer+inode_relative+44, &inode->doubleIndPtr, 4);

    write_block(superBloco.InodeBlock + block_relative, buffer, superBloco);
    set_on_bitmap(id_inode, 1, INODE, superBloco);
    return 0;
}

int delete_inode(int id_inode)
{
    if (id_inode >= superBloco.NofBlocks) {
        printf("Tentando deletar inode %d que nao existe.\n", id_inode);
        return -1;
    }

    return set_on_bitmap(id_inode, 0, INODE, superBloco);
}

struct t2fs_inode read_i_node(int id_inode)
{
    struct t2fs_inode inode;//={0};

    int block_relative = ((id_inode) * INODE_SIZE) / superBloco.BlockSize;
//    printf("block relative: %d\n", block_relative);
    int inode_relative = (((id_inode) * INODE_SIZE) % superBloco.BlockSize);
//    printf("inode relative: %d\n", inode_relative);

    //le bloco do inodes
    char buffer[superBloco.BlockSize];
    read_block(superBloco.InodeBlock + block_relative, buffer, superBloco);

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

void print_sector(int id_block)
{
    char buffer[superBloco.BlockSize];
    read_block(id_block, buffer, superBloco);

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

void printSuperBloco()
{
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

int is_letter(char c)
{
    if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
        return 1;
    } else {
        return 0;
    }
}

int is_number(char c)
{
    if (c >= 48 && c <= 57) {
        return 1;
    } else {
        return 0;
    }
}

int is_path_consistent(char *strpath)
{
    int i = 0;
    int namelength = 0;

    while (strpath[i] != '\0') {
        if (strpath[i] == '/') {
            i++;
            namelength = 0;
        } else {
            // tamanho do nome
            if (namelength >= 30) {
                printf("ERRO: Caminho invalido, nome com mais de 30 caracteres.\n");
                return 0;
            }
            if (is_number(strpath[i]) || is_letter(strpath[i])) {
                i++;
                namelength++;
            } else {
                // permitir /dir/./ ou /dir/../
                if (strpath[i] == '.') {
                    // '.' apenas unico caractere do nome, 1 ou 2 vezes:
                    if (namelength == 0 || namelength == 1) {
                        if (strpath[i - namelength] == '.') {
                            // se seguido de /
                            if (strpath[i - namelength + 1] == '/' ||
                                strpath[i - namelength + 1] == '\0') {
                                i++;
                                namelength++;
                                continue;
                            }
                            if (strpath[i - namelength + 1] == '.') {
                                if (strpath[i - namelength + 2] == '/' ||
                                    strpath[i - namelength + 2] == '\0') {
                                    i++;
                                    namelength++;
                                    continue;
                                }
                            }
                        }
                    }
                }
                printf("ERRO: Caminho invalido, nome contem caractere invalido.\n");
                return 0;
            }
        }
    }
    return 1;
}

/**
 *  Dado um nome de arquivo, verifica se o mesmo é consistente, isto é:
 *  tem no máximo 30 caracteres alfanuméricos.
 */
int is_name_consistent(char *name)
{
    int i = 0;
    while (name[i] != '\0') {
        if (is_number(name[i]) || is_letter(name[i])) {
            i++;
        } else {
            printf("ERRO: Nome passado contem um caractere invalido\n");
            return 0;
        }
    }
    if (i > 30) {
        printf("ERRO: Nome passado e maior que 30 caracteres.\n");
        return 0;
    }

    return 1;
}

/**
 *  Funcao que adiciona um arquivo aberto na lista global de arquivos abertos,
 *  como ultimo elemento.
 *  Note que a lista global (open_files) é uma lista simplesmente encadeada.
 */
// TODO: Encontrar possiveis erros
int add_opened_file_to_list(OPEN_FILE *open_file, file_type type)
{
    OPEN_FILE *searcher;
    if (type == FILE_TYPE) {
        printf("adding opened file to list\n");
        if (open_files->first == NULL) { // Lista está vazia
            open_files->first = open_file;
            open_files->size++;

            open_file->next   = NULL;
            return 0;
        }

        searcher = open_files->first;

        while (searcher->next != NULL) { // Procura ultimo elemento da lista
            searcher = searcher->next;
        }

        searcher->next = open_file;
        open_files->size++;
        return 0;
    } else {
        printf("adding opened dir to list\n");
        if (open_directories->first == NULL) {
            open_directories->first = open_file;
            open_directories->size++;
            open_file->next = NULL;
            return 0;
        }

        searcher = open_directories->first;

        while (searcher->next != NULL) {
            searcher = searcher->next;
        }
        searcher->next = open_file;
        open_directories->size++;
        return 0;
    }
}

/**
 *  Funcao que seta todas entradas do inode para 0x0FFFFFFFF
 */
void initialize_inode(struct t2fs_inode *inode)
{
    int i;
    for (i = 0; i < 10; i++) {
        inode->dataPtr[i] = INVALID_POINTER;
    }
    inode->singleIndPtr = INVALID_POINTER;
    inode->doubleIndPtr = INVALID_POINTER;
}

/************************************************/
/* Definicao das funcoes principais do trabalho */
/************************************************/

int identify2(char *name, int size)
{
    checkSuperBloco();

    int i = 0;
    char ids[] = "Alexandre Leuck (...), Gianei Sebastiany (213502)"
        " e Leonardo Hahn (207684)\0";

    while (ids[i] != '\0' && i < size - 1) {
        name[i] = ids[i];
        i++;
    }
    if (size < strlen(ids)) {
        return -1;
    }
    name[size - 1] = '\0';

    return 0;
}

/**
 *  Funcao que cria um novo arquivo no diretorio atual, dado o seu nome.
 */
FILE2 create2(char *filename)
{
    checkSuperBloco();

    //acha inode
    //char *before_filename = get_string_before_last_bar(filename);
    //struct t2fs_inode creating_inode;
    // if (*before_filename != 0){ //se nome não tem barras
    //     find_inode_from_path(superBloco, filename, current_dir, &creating_inode);
    // } else {
    //     creating_inode = current_dir;
    // }

    //char *striped_filename = get_string_after_bar(filename);

    //if (!is_name_consistent(striped_filename)) {
    if (!is_name_consistent(filename)) {
        printf("ERRO: nome inconsistente\n");
        return -1;
    }

    struct t2fs_record *new_file_record;
    struct t2fs_inode *new_file_inode;

    int idx = get_free_bit_on_bitmap(INODE, superBloco);
    //printf("Primeiro indice livre de inode e %d", idx);

    // TODO: No momento só guarda no diretório corrente,
    //       não permite passagem do caminho completo.

    // Cria o record para o arquivo
    new_file_record                 = malloc(sizeof *new_file_record);
    new_file_record->TypeVal        = TYPEVAL_REGULAR;
    new_file_record->i_node         = idx;
    new_file_record->blocksFileSize = 1;         // ocupa 1 inode quando criado
    new_file_record->bytesFileSize  = 0;        // TODO-: 31 bytes?? ou 0? --> tamanho do arquivo = tamanho bloco x blocs usados
    //memcpy(new_file_record->name, striped_filename, 31);
    memcpy(new_file_record->name, filename, 31);

    // Cria inode para arquivo
    new_file_inode = malloc(sizeof *new_file_inode);
    initialize_inode(new_file_inode);
    write_inode(idx, new_file_inode);
    free(new_file_inode);

    struct t2fs_inode current_dir = read_i_node(current_path.current->record.i_node);

    //add_record_to_inode(creating_inode, *new_file_record);
    add_record_to_inode(&current_dir, *new_file_record);
    write_inode(current_path.current->record.i_node, &current_dir);

    return open2(filename);
}

int delete2(char *filename)
{
    checkSuperBloco();
    struct t2fs_inode inode = read_i_node(0);
    print_inode(inode);
    // Escreve inode igual ao inode 0 no inode 1
    write_inode(1, &inode);

    struct t2fs_inode inode1 = read_i_node(1);
    print_inode(inode1);

    //printf("\n%d, %d\n", superBloco.InodeBlock, superBloco.FirstDataBlock);
    //print_inode(*read_i_node(0));
    //printf("\nBLOCKS\n");
    //printf("inode: %d, block: %d\n", superBloco.InodeBlock, superBloco.FirstDataBlock);
    //print_bitmap(BLOCK, superBloco);
    //printf("\nINODES\n");
    print_bitmap(INODE, superBloco);
    return 0;
}

FILE2 open2(char *filename)
{
    if (!is_name_consistent(filename)) {
        printf("ERRO: nome inconsistente\n");
        return -1;
    }
    checkSuperBloco();
    if ((open_files->size + open_directories->size) == MAX_OPEN_FILES) {
        printf("ERRO: numero maximo de arquivos abertos (20)\n");
        return -1;
    }
    struct t2fs_record *file_record = malloc(sizeof(*file_record));
    struct t2fs_inode current_dir = read_i_node(current_path.current->record.i_node);
    if (find_record_in_inode(current_dir, filename, file_record) == -1) {
        free(file_record);
        printf("ERRO: registro nao encontrado\n");
        return -1;
    }

    OPEN_FILE *open_file = malloc(sizeof(*open_file));
    open_file->record = file_record;
    open_file->inode = malloc(sizeof(struct t2fs_inode));
    *(open_file->inode) = read_i_node(file_record->i_node);
    open_file->position = 0;
    open_file->handle = current_handle;
    open_file->next = NULL;
    open_file->parent_inode = current_path.current->record.i_node;
    add_opened_file_to_list(open_file, FILE_TYPE);

    current_handle++;
    return current_handle - 1;
}

/**
 *  Funcao que fecha um arquivo aberto dado o seu handle.
 *  No momento nao existe distincao entre close2 e closedir2
 */
int close2(FILE2 handle)
{
    checkSuperBloco();

    OPEN_FILE *searcher = open_files->first;

    if (searcher == NULL) {
        printf("ERRO: Nao e possivel fechar arquivo %d, o mesmo nao existe\n", handle);
        return -1;
    } else if (searcher->handle == handle) { // se for o primeiro elemento
        open_files->first = searcher->next;
        open_files->size--;
        free(searcher->inode);
        free(searcher->record);
        free(searcher);
        printf("Arquivo %d fechado com sucesso\n", handle);
        return 0;
    }

    // Enquanto nao chegar no fim da lista e handle do arquivo nao for igual ao handle passado
    while (searcher->handle != handle) {
        if (searcher->next == NULL) {
            printf("ERRO: Handle %d nao aponta para um arquivo aberto.\n", handle);
            return -1;
        }
        searcher = searcher->next;
    }

    if (searcher->next->next == NULL) { // Se searcher->next for o ultimo elemento da lista
        free(searcher->next->record);
        free(searcher->next->inode);
        free(searcher->next);
        searcher = NULL;
        open_files->size--;
        printf("Arquivo %d fechado com sucesso\n", handle);
        return 0;
    }
    OPEN_FILE *aux = searcher->next;
    searcher->next = searcher->next->next;
    free(aux->record);
    free(aux->inode);
    free(aux);
    open_files->size--;
    printf("Arquivo %d fechado com sucesso\n", handle);
    return 0;
}

int read2(FILE2 handle, char *buffer, int size)
{
    checkSuperBloco();

    OPEN_FILE *file = get_file_from_list(handle, FILE_TYPE);

    unsigned int first_block = file->position / superBloco.BlockSize;
    unsigned int blocks_to_read = ((file->position + size) / superBloco.BlockSize) + 1;
    unsigned int b;
    unsigned int last_position = get_position_eof(file);

    if ((file->position + size) > last_position) {
        printf("ERRO: Tentando ler apos final do arquivo\n");
    }
    // Le todos os blocos que fazem parte de buffer
    int last_i = 0;
    int bytes_read = 0;

    for (b = 0; b < blocks_to_read; b++) {
        int id_block = get_block_id_from_inode(first_block+b, file->inode);
        char buf[superBloco.BlockSize];
        read_block(id_block, buf, superBloco);

        unsigned int i = 0;
        if (b != (blocks_to_read - 1)) { // Se nao for o ultimo bloco
            for (i = 0; i < superBloco.BlockSize-1; i++) {
                buffer[i + last_i] = buf[i];
                bytes_read++;
            }
        } else {                         // Se for o ultimo bloco
            while (bytes_read != size) {
                buffer[bytes_read + last_i] = buf[bytes_read];
                bytes_read++;
            }
        }
        last_i = i;
    }

    buffer[size-1] = '\0';
    return 0;
}

/**
 *  Dado o id de um bloco, retorna um array com os indices contidos nesse bloco.
 *  O array é alocado dinamicamente, entao é responsabilidade do chamador desalocar
 *  a memória.
 */
DWORD* get_indices(int id_block)
{
    DWORD *indices = malloc(get_num_indices_in_block() * sizeof(*indices));

    char buffer[superBloco.BlockSize];
    read_block(id_block, buffer, superBloco);

    unsigned int i;
    for (i = 0; i < get_num_indices_in_block(); i++) {
        //indices[i] = (DWORD) buffer[i * sizeof(DWORD)];
        memcpy(indices+i, buffer+(i*4), 4);
    }

    return indices;
}

/**
 *  Dado um indice e um inode, retorna o indice do bloco está relacionado a ele no inode.
 *  Se o indice nao for encontrado, retorna -1.
 *
 *  Ex.: Se passado o indice 9, retornara o 9 indice do bloco relacionado a esse indice,
 *       no caso inode->dataPtr[9]. O mesmo é valido para blocos indiretos.
 */
int get_block_id_from_inode(int relative_index, struct t2fs_inode *inode)
{
    DWORD pointer;
    if (relative_index < 10) {
        pointer = inode->dataPtr[relative_index];
        if (pointer == INVALID_POINTER) {
            printf("WARNING: inode nao contem bloco relativo de indice %d\n", relative_index);
            return -1;
        } else {
            return pointer;
        }
    }
    if (inode->singleIndPtr == INVALID_POINTER) {
        printf("ERRO: inode nao contem bloco relativo de indice %d", relative_index);
        return -1;
    }
    int id_singleInd_block = relative_index - 10;
    if (id_singleInd_block >= 0 && id_singleInd_block < get_num_indices_in_block()) { // Indice pertence aos blocos indiretos
        DWORD *indices = get_indices(inode->singleIndPtr);
        pointer = indices[id_singleInd_block];

        if (pointer == INVALID_POINTER) {
            printf("ERRO: inode nao contem bloco relativo de indice %d", relative_index);
            return -1;
        }

        free(indices);
        return pointer;
    }
    // Se chegar aqui, esta nos blocos com duas indireções
    if (inode->doubleIndPtr == INVALID_POINTER) {
        printf("ERRO: inode nao contem bloco relativo de indice %d", relative_index);
        return -1;
    }
    printf("JESUS CRISTO, AINDA NAO TA IMPLEMENTADO PRA ARQUIVOS TAO GRANDES\n");
    //int id = relative_index - 10 - N_OF_INDICES;
    //DWORD *first_lvl_id = get_indices(inode->doubleIndPtr);
    return -1;
}

/**
 *  Dado um índice de um bloco. Seta as entradas de todos os indices
 *  para 0x0FFFFFFFF. Sendo que o numero de indices e dado por (get_num_indices_in_block())
 */
void init_indices_block(int id_block)
{
    char buffer[superBloco.BlockSize];
    DWORD invalid = 0x0FFFFFFFF;

    int i;
    for (i = 0; i < get_num_indices_in_block(); i++) {
        memcpy(buffer+(i*sizeof(DWORD)), (char*)&invalid, sizeof(DWORD));
    }

    write_block(id_block, buffer, superBloco);
}

void init_data_block(int id_block)
{
    char buffer[superBloco.BlockSize];
    DWORD zero = 0x0;

    int i;
    for (i = 0; i < get_num_indices_in_block(); i++) {
        memcpy(buffer+(i*sizeof(DWORD)), (char*)&zero, sizeof(DWORD));
    }

    write_block(id_block, buffer, superBloco);
}

void init_records_block(int id_block)
{
    char buffer[superBloco.BlockSize];
    DWORD zero = -1;

    int i;
    for (i = 0; i < get_num_indices_in_block(); i++) {
        memcpy(buffer+(i*sizeof(DWORD)), (char*)&zero, sizeof(DWORD));
    }

    write_block(id_block, buffer, superBloco);
}

void print_indices(int id_block)
{
    DWORD *indices = get_indices(id_block);
    int i;
    printf("\n");
    for (i = 0; i < get_num_indices_in_block(); i++) {
        printf("id %d = %d\n", i, indices[i]);
    }
    free(indices);
}

int write_indices(int id_block, DWORD *indices)
{
    char buffer[superBloco.BlockSize];
    int i;
    for (i = 0; i < get_num_indices_in_block(); i++) {
        memcpy(buffer+(i*sizeof(DWORD)), indices+i, sizeof(DWORD));
    }

    write_block(id_block, buffer, superBloco);
    return 0;
}

int allocate_block(bitmap_type type)
{
    int new_id = get_free_bit_on_bitmap(type, superBloco);
    set_on_bitmap(new_id, 1, type, superBloco);
    return new_id;
}

int free_block(int id, bitmap_type type)
{
    return set_on_bitmap(id, 0, type, superBloco);
}

int allocate_block_on_inode(struct t2fs_inode *inode)
{
    int i;
    for (i = 0; i < 10; i++) {
        if (inode->dataPtr[i] == INVALID_POINTER) {
            int new_ind = allocate_block(BLOCK);
            inode->dataPtr[i] = new_ind;
            if (new_ind == -1) {
                return -1;
            }
            return 0;
        }
    }

    if (inode->singleIndPtr == INVALID_POINTER) { // Aloca bloco de indices
        int new_ind = allocate_block(BLOCK);
        inode->singleIndPtr = new_ind;
        if (new_ind == -1) {
            return -1;
        }
        init_indices_block(inode->singleIndPtr);

        DWORD *indices = get_indices(inode->singleIndPtr);
        new_ind = allocate_block(BLOCK);
        indices[0] = new_ind;
        if (new_ind == -1) {
            return -1;
        }

        write_indices(inode->singleIndPtr, indices);
        free(indices);
        return 0;
    }
    // Se ja existe bloco de indices
    DWORD *indices = get_indices(inode->singleIndPtr);
    //print_indices(inode->singleIndPtr);
    for (i = 0; i < get_num_indices_in_block(); i++) {
        if (indices[i] == INVALID_POINTER) {
            int new_ind = allocate_block(BLOCK);
            indices[i] = new_ind;
            if (new_ind == -1) {
                return -1;
            }
            write_indices(inode->singleIndPtr, indices);
            free(indices);
            return 0;
        }
    }
    // Nenhum indice livre em single ind.
    if (inode->doubleIndPtr == INVALID_POINTER) {
        int new_ind = allocate_block(BLOCK);
        inode->doubleIndPtr = new_ind;
        if (new_ind == -1) {
            return -1;
        }
        init_indices_block(inode->doubleIndPtr);
    }
    DWORD *double_indices = get_indices(inode->doubleIndPtr);
    for (i = 0; i < get_num_indices_in_block(); i++) {
        if (double_indices[i] == INVALID_POINTER) {
            int new_ind = allocate_block(BLOCK);
            double_indices[i] = new_ind;
            if (new_ind == -1) {
                return -1;
            }
            init_indices_block(double_indices[i]);
        }
        int j;
        DWORD *sing_indices = get_indices(double_indices[i]);
        for (j = 0; j < get_num_indices_in_block(); j++) {
            if (sing_indices[j] == INVALID_POINTER) {
                int new_ind = allocate_block(BLOCK);
                sing_indices[j] = new_ind;
                if (new_ind == -1) {
                    return -1;
                }
                write_indices(double_indices[i], sing_indices);
                free(sing_indices);
                return 0;
            }
        }
    }
    return -1;
}

int get_last_abstract_block_from_inode(struct t2fs_inode *inode)
{
    int i = 0;
    while (get_block_id_from_inode(i, inode) != -1) {
        i++;
    }
    return i-1;
}

int get_position_eof(OPEN_FILE *file)
{
    return file->record->bytesFileSize;
}

int set_position_eof(OPEN_FILE *file)
{
    file->position = file->record->bytesFileSize;
    return 0;
}

/**
 *  Funcao que dado o handle do arquivo, escreve no mesmo o conteudo
 *  do buffer.
 */
int write2(FILE2 handle, char *buffer, int size)
{
    checkSuperBloco();

    OPEN_FILE    *file          = get_file_from_list(handle, FILE_TYPE);
    unsigned int first_block_id = file->position / superBloco.BlockSize;
    int          blocks_to_read = (((file->position + size) - 1) / superBloco.BlockSize) + 1;
    char         to_write[blocks_to_read][superBloco.BlockSize];

    printf("block_size: %d\n", superBloco.BlockSize);
    printf("blocks_to_read: %d\n", blocks_to_read);

    int b;
    for (b = 0; b < blocks_to_read; b++) {
        int real_block_id = get_block_id_from_inode(first_block_id+b, file->inode);
        if (real_block_id == -1) { // Bloco ainda não está alocado ao arquivo
            if (allocate_block_on_inode(file->inode) == -1) {
                printf("ERRO: Falha ao tentar alocar bloco a arquivo.\n");
                return -1;
            }
            file->record->blocksFileSize++;
            real_block_id = get_block_id_from_inode(first_block_id+b, file->inode);
        }
        // Le o bloco necessario para a escrita
        read_block(real_block_id, &to_write[b][0], superBloco);
    }
    // Escreve buffer passado
    unsigned int j = 0;
    int bytes_written = 0;
    for (b = 0; b < blocks_to_read; b++) {
        for (j = 0; j < superBloco.BlockSize-1; j++) {
            if (bytes_written == size) {
                break;
            }
            if ((j + (b*superBloco.BlockSize)) >= file->position) {
                to_write[b][j] = buffer[bytes_written];
                bytes_written++;
                file->position++;
                if (file->position > file->record->bytesFileSize) {
                    file->record->bytesFileSize++; // Atualiza tamanho em bytes do arquivo.
                }
            }
        }
        to_write[b][superBloco.BlockSize-1] = '\0';  // Força fim de linha
        file->position++;
    }
    int i;
    for (b = 0; b < blocks_to_read; b++) {
        for (i = 0; i < superBloco.BlockSize-1; i++) {
            printf("%c", to_write[b][i]);
        }
        printf("\n");
    }
    for (b = 0; b < blocks_to_read; b++) {
        int id = get_block_id_from_inode(first_block_id + b, file->inode);
        write_block(id, &to_write[b][0], superBloco);
    }
    write_inode(file->record->i_node, file->inode);
    struct t2fs_inode parent_inode = read_i_node(file->parent_inode);
    add_record_to_inode(&parent_inode, *(file->record));
    return 0;
}

int seek2(FILE2 handle, unsigned int offset)
{
    // TODO: Checar por erros
    checkSuperBloco();

    OPEN_FILE *file = get_file_from_list(handle, FILE_TYPE);

    if (offset == (unsigned int) -1) {
        set_position_eof(file);
        return 0;
    }

    file->position = offset;

    return 0;
}

int mkdir2(char *pathname)
{
    checkSuperBloco();
    // path where the dir will be created
    PATH dest_path;
    char name[31];
    memset(&dest_path, 0, sizeof(PATH));

    if (!is_path_consistent(pathname)) {
        printf("ERRO: caminho inconsistente\n");
        return -1;
    }

    int pathlength = strlen(pathname) + 1;
    char *path = malloc(sizeof(char) * pathlength);
    strncpy(path, pathname, pathlength);

    // allow relative paths
    copy_path(&dest_path, &current_path);

    int bar_index = last_index_of('/', path);


    if (bar_index != -1) {
        strncpy(name, &(path[bar_index + 1]), 30);
        path[bar_index + 1] = '\0';
        chdir2_generic(&dest_path, path);
    } else {
        strncpy(name, path, sizeof(name));
    }
    free(path);
    //printf("dirname: %s\n", name);

    // inode for current dir
    struct t2fs_inode current_dir = read_i_node(dest_path.current->record.i_node);

    // diretorio existente?
    struct t2fs_record new_directory_record;
    if (find_record_in_inode(current_dir, name, &new_directory_record) != -1) {
        free_path(&dest_path);
        printf("mkdir2: cannot create directory ‘%s’: File exists\n", name);
        return -1;
    }


    int idx = get_free_bit_on_bitmap(INODE, superBloco);
    //printf("Primeiro indice livre de inode e %d\n", idx);

    // Cria o record para o diretorio
    //new_directory_record = malloc(sizeof *new_directory_record);
    new_directory_record.TypeVal        = TYPEVAL_DIRETORIO;
    new_directory_record.i_node         = idx;
    new_directory_record.blocksFileSize = 1;         // ocupa 1 inode quando criado
    new_directory_record.bytesFileSize  = 0;        // TODO-: 31 bytes?? ou 0? --> tamanho do arquivo = tamanho bloco x blocs usados
    //memcpy(new_directory_record->name, striped_filename, 31);
    memcpy(new_directory_record.name, &name, 31);

    //printf("*add new dir record to parents inode\n");
    // get inode where the dir is being created
    if (add_record_to_inode(&current_dir, new_directory_record) != 0) {
        free_path(&dest_path);
        printf("ERRO\n");
        return -1;
    }
    write_inode(dest_path.current->record.i_node, &current_dir);

    // Cria inode para arquivo
    struct t2fs_inode new_directory_inode;
    initialize_inode(&new_directory_inode);

    //printf("*add record to self `.`\n");
    // Cria o record para o self '.'
    struct t2fs_record self_record;
    self_record.TypeVal        = TYPEVAL_DIRETORIO;
    self_record.i_node         = idx;
    self_record.blocksFileSize = 1;         // ocupa 1 inode quando criado
    self_record.bytesFileSize  = RECORD_SIZE * 2;        // ocupa dois records
    memcpy(self_record.name, ".\0", 2);
    if (add_record_to_inode(&new_directory_inode, self_record) != 0) {
        free_path(&dest_path);
        printf("ERRO\n");
        return -1;
    }

    //printf("*add record to self `..`\n");
    // Cria o record para o pai '..'
    struct t2fs_record father_record;
    int idx_pai = dest_path.current->record.i_node;
    father_record.TypeVal        = TYPEVAL_DIRETORIO;
    father_record.i_node         = idx_pai;
    father_record.blocksFileSize = -1;         // TODO tamanho de blocos do dir pai?
    father_record.bytesFileSize  = -1;        //  TODO tamanho do dir pai? imagina atualizar tudo isso
    memcpy(father_record.name, "..\0", 3);
    add_record_to_inode(&new_directory_inode, father_record);

    free_path(&dest_path);

    write_inode(idx, &new_directory_inode);
    return 0;
}

int rmdir2(char *pathname)
{
    struct t2fs_record records[get_records_in_block()];

    checkSuperBloco();
    if (!is_path_consistent(pathname)) {
        printf("Caminho passado nao e consistente\n");
        return -1;
    }

    PATH path;
    memset(&path, 0, sizeof(PATH));

    // allows relative pathnames
    copy_path(&path, &current_path);

    if (chdir2_generic(&path, pathname) != 0) {
        return -1;
    }

    struct t2fs_inode inode = read_i_node(path.current->record.i_node);

    if (inode.dataPtr[0] == INVALID_POINTER) {
        printf("rmdir2: Diretorio nao tem records.\n");
        return -1;
    }

    read_records(inode.dataPtr[0], records);

    if (strcmp(records[0].name, ".") != 0) {
        printf("rmdir2: Diretorio nao tem self_record na posicao 0.\n");
        return -1;
    }

    if (strcmp(records[1].name, "..") != 0) {
        printf("rmdir2: Diretorio nao tem self_record na posicao 1.\n");
        return -1;
    }

    if (records[2].TypeVal != TYPEVAL_INVALIDO) {
        printf("rmdir2: Diretorio nao esta vazio.\n");
        return -1;
    }

    free_block(inode.dataPtr[0], BLOCK);

    inode = read_i_node(path.current->previous->record.i_node);
    remove_record_from_inode(&inode, path.current->record.name);

    write_inode(path.current->previous->record.i_node, &inode);

    delete_inode(path.current->record.i_node);

    return 0;
}

DIR2 opendir2(char *pathname)
{
    checkSuperBloco();
    if (!is_path_consistent(pathname)) {
        printf("ERRO: caminho inconsistente\n");
        return -1;
    }

    PATH path;
    memset(&path, 0, sizeof(PATH));

    // allows relative pathnames
    printf("opendir2: copy_path:\n");
    copy_path(&path, &current_path);

    printf("opendir2: chdir2_generic:\n");
    if (chdir2_generic(&path, pathname) != 0) {
        return -1;
    }

    printf("opendir2: check open count:\n");
    if ((open_files->size + open_directories->size) == MAX_OPEN_FILES) {
        printf("ERRO: numero maximo de arquivos abertos (20)\n");
        return -1;
    }
    printf("opendir2: mallocs:\n");
    OPEN_FILE *open_file = malloc(sizeof *open_file);
    open_file->inode    = malloc(sizeof(struct t2fs_inode));
    printf("opendir2: read_i_node:\n");
    *(open_file->inode)    = read_i_node(path.current->record.i_node);
    open_file->position = 0;
    open_file->handle   = current_handle;
    open_file->next     = NULL;
    if (path.current->previous)
        open_file->parent_inode = path.current->previous->record.i_node;
    else
        open_file->parent_inode = 0;
    printf("opendir2: read_i_node:\n");
    add_opened_file_to_list(open_file, DIR_TYPE);
    current_handle++;
    return current_handle-1;
}

/**
 *  Funcao que retorna um arquivo aberto dado o seu handle.
 *  Retorna NULL caso nao encontre o arquivo.
 */
OPEN_FILE* get_file_from_list(int handle, file_type type)
{
    OPEN_FILES *list;
    OPEN_FILE *searcher;
    if (type == FILE_TYPE) {
        list = open_files;
    } else {
        list = open_directories;
    }

    if (list->first == NULL) {
        printf("ERRO: Tentando pegar um arquivo de uma lista vazia\n");
        return NULL;
    }

    searcher = list->first;

    while (searcher->handle != handle) {
        if (searcher->next == NULL) {
            printf("ERRO: Arquivo nao encontrado\n");
            return NULL;
        }
        searcher = searcher->next;
    }
    return searcher;
}

int readdir2(DIR2 handle, DIRENT2 *dentry)
{
    checkSuperBloco();
    struct t2fs_record records[get_records_in_block()]; //sempre há X records por bloco
    OPEN_FILE* workingFile = get_file_from_list(handle, DIR_TYPE);

    if (workingFile == NULL) {
        return -1;
    }

    struct t2fs_inode dir_inode = read_i_node(workingFile->record->i_node);
    read_records(dir_inode.dataPtr[0], records);

    //read_records(workingFile->inode->dataPtr[0], records); //lê records apontados pelo primeiro ponteiro de I-node

    if (records[workingFile->position].TypeVal == 1 || records[workingFile->position].TypeVal == 2) { //entrada válida
        memcpy(dentry->name,records[workingFile->position].name,31);
        dentry->fileType = records[workingFile->position].TypeVal;
        dentry->fileSize = records[workingFile->position].bytesFileSize;

        workingFile->position++;

        return 0;
    } else { //não ha registro aqui
        for (workingFile->position++; workingFile->position < get_records_in_block(); workingFile->position++) {
            if (records[workingFile->position].TypeVal == 1 || records[workingFile->position].TypeVal == 2) {
                memcpy(dentry->name,records[workingFile->position].name,31);
                dentry->fileType = records[workingFile->position].TypeVal;
                dentry->fileSize = records[workingFile->position].bytesFileSize;
                workingFile->position++;
                return 0;
            }
        }
        //chegou no fim do primeiro apontador do I-node
        if (records[0].blocksFileSize == 1) { //acessa o tamanho em blocos do diretório . (ou seja ele mesmo)
            return -1;
        } else {
            //TODO
            // inode aponta para mais de um bloco
            //acho q é possível identificar isso pelo tamanho do Inode... ou algo de tamanho (como no teste acima)
            //procurar se há mais blocos apontados no I-Node
            printf("Procura de diretorios grandes ainda nao implementado\n");
            return -1; //força erro
        }
    }

    return 0;
}

int closedir2(DIR2 handle)
{
    checkSuperBloco();
    return 0;
}

int free_path(PATH *path)
{
    PATH_NODE *dir;
    while (path->current != NULL) {
        dir = path->current;
        path->current = path->current->previous;
        free(dir);
    }
    return 0;
}

int copy_path(PATH *dest, const PATH *origin)
{
    free_path(dest);

    PATH_NODE *node = NULL, *copynode = NULL;

    if (origin->current != NULL) {
        node = origin->current;
        dest->current = malloc(sizeof(PATH_NODE));
        copynode = dest->current;
        copynode->record = node->record;
        copynode->previous = NULL;
    }

    while (node->previous != NULL) {
        copynode->previous = malloc(sizeof(PATH_NODE));
        copynode = copynode->previous;
        node = node->previous;
        copynode->record = node->record;
        copynode->previous = NULL;
    }

    return 0;
}

// frees path and set it to '/'
int chdir2_root(PATH *current_path)
{

    free_path(current_path);

    current_path->current = malloc(sizeof(PATH_NODE));
    current_path->current->previous = NULL;
    current_path->current->record.name[0] = '\0';
    current_path->current->record.TypeVal = TYPEVAL_DIRETORIO;
    current_path->current->record.i_node = 0;
    return 0;
}

// advaces one directory in path to dirname, accepts . (do nothing) or .. (go back 1 directory);
int chdir2_simple(PATH *current_path, char *dirname)
{
    // . : stay (do nothing)
    if (strcmp(dirname, ".\0") == 0) {
        return 0;
    }

    PATH_NODE *dir = NULL;
    // .. : go back one dir
    if (strcmp(dirname, "..\0") == 0) {
        if (current_path->current->previous != NULL) {
            dir = current_path->current;
            current_path->current = dir->previous;
            free(dir);
        }
        return 0;
    }

    // invalid dirname?
    if (!is_name_consistent(dirname)) {
        return -1;
    }

    struct t2fs_inode dir_inode;
    struct t2fs_record dir_record;

    dir_inode = read_i_node(current_path->current->record.i_node);
    if (find_record_in_inode(dir_inode, dirname, &dir_record) != -1) {
        if (dir_record.TypeVal != TYPEVAL_DIRETORIO) {
            printf("ERRO: '%s' nao e um diretorio.\n", dir->record.name);
            return -1;
        }
        dir = malloc(sizeof(PATH_NODE));
        memcpy(&(dir->record), &dir_record, sizeof(struct t2fs_record));
        dir->previous = current_path->current;
        current_path->current = dir;
        return 0;
    }

    printf("ERRO: Diretorio `%s` nao encontrado.\n", dirname);
    return -1;
}

int chdir2_generic (PATH *current_path, char *pathname)
{

    if(!is_path_consistent(pathname)) {
        return -1;
    }
    if (pathname[0] == '/') {
        chdir2_root(current_path);
    }
    char *path;
    int pathlength = strlen(pathname) + 1;
    path = malloc(sizeof(char)*pathlength);
    strncpy(path, pathname, pathlength);
    char *dirname;
    dirname = strtok(path, "/");
    while (dirname != NULL) {
        if (chdir2_simple(current_path, dirname) != 0) {
            free(path);
            return -1;
        }
        dirname = strtok(NULL, "/");
    }
    free(path);
    return 0;
}

int chdir2(char *pathname)
{
    checkSuperBloco();
    return chdir2_generic(&current_path, pathname);
}

int getcwd2_helper(PATH_NODE *path_node, char *pathname, int size) {
    int index = 0;
    if (path_node->previous) {
        index = getcwd2_helper(path_node->previous, pathname, size);
        pathname[index++] = '/';
    }
    int i = 0;
    while (path_node->record.name[i] != '\0' && (i + index + 1) < size) {
        pathname[i + index] = path_node->record.name[i];
        i++;
    }
    pathname[i + index] = '\0';
    return index + i;
}

int getcwd2(char *pathname, int size)
{
    checkSuperBloco();
    getcwd2_helper(current_path.current, pathname, size);
    return 0;
}

void checkSuperBloco()
{
    if (superBloco.DiskSize == 0) {

        char buffer_super_bloco[SECTOR_SIZE];

        // Inicializa lista de diretorios e arquivos
        open_files = malloc(sizeof *open_files);
        open_files->size = 0;
        open_files->first = NULL;
        open_directories = malloc(sizeof *open_directories);
        open_directories->size = 0;
        open_directories->first = NULL;

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

        // TODO: migrar para current_path para permitir 'chdir("..");'
        chdir2_root(&current_path);
        current_dir = read_i_node(current_path.current->record.i_node);
    }
}
