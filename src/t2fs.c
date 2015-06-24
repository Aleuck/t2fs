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
    struct t2fs_inode *inode;
    int position;
    int handle;
    struct open_file *next;
} OPEN_FILE;

typedef struct {
    int size;
    OPEN_FILE *first;
} OPEN_FILES;

struct t2fs_superbloco superBloco;

int current_handle = 0;       // Indica o valor do handle do prox. arquivo aberto
OPEN_FILES *open_files;       // Lista encadeada dos arquivos abertos
OPEN_FILES *open_directories; // Lista encadeada dos diretorios abertos
struct t2fs_inode current_dir;

/* Prototipo de Funcoes */

void checkSuperBloco();
void print_record(struct t2fs_record record);
OPEN_FILE* get_file_from_list(int handle, file_type type);
int get_ptrs_in_block();
void print_indices(int id_block);

/***********************************/
/* Definicao do corpo das Funcoes **/
/***********************************/
int get_num_indices()
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

int add_record_to_data_ptr_array(DWORD *dataPtr, int dataPtrLength, struct t2fs_record file_record)
{
    int records_in_block = get_records_in_block();
    struct t2fs_record records[records_in_block];
    DWORD id_block;
    int dataPtr_index;
    int record_index;

    for (dataPtr_index = 0; dataPtr_index < dataPtrLength; dataPtr_index++) {
        id_block = dataPtr[dataPtr_index];
        if (id_block == 0x0FFFFFFFF) {
            // TODO: criar novo bloco?
            continue;
        }
        read_records(id_block, records);
        for (record_index = 0; record_index < records_in_block; record_index++) {
            if (records[record_index].TypeVal == TYPEVAL_INVALIDO) {
                // encontrada posição livre
                break;
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

int add_record_to_inode(struct t2fs_inode inode, struct t2fs_record file_record)
{
    // TODO: Por enquanto pega somente o primeiro inode.
    int ptrs_in_block = get_ptrs_in_block();
    DWORD singleInd[ptrs_in_block];
    DWORD doubleInd[ptrs_in_block];
    int ptr_index;

    // tenta inserir nos ponteiros diretos
    if (add_record_to_data_ptr_array(inode.dataPtr, 10, file_record) == 0) {
        return 0;
    }
    // tenta usar indireção simples
    if (inode.singleIndPtr != 0x0FFFFFFFF) {
        read_block(inode.singleIndPtr, (char*) singleInd, superBloco);
        if (add_record_to_data_ptr_array(singleInd, ptrs_in_block, file_record) == 0) {
            return 0;
        }
    } else {
        // TODO: alocar bloco de indireção simples?
    }
    // tenta usar indireção dupla
    if (inode.doubleIndPtr != 0x0FFFFFFFF) {
        read_block(inode.doubleIndPtr, (char*) doubleInd, superBloco);
        for (ptr_index = 0; ptr_index < ptrs_in_block; ptr_index++) {
            read_block(doubleInd[ptr_index], (char*) singleInd, superBloco);
            if (add_record_to_data_ptr_array(singleInd, ptrs_in_block, file_record) == 0) {
                return 0;
            }
        }
    } else {
        // TODO: alocar bloco de indireção dupla?
    }
    return -1;
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

int get_ptrs_in_block()
{
    return superBloco.BlockSize / sizeof(DWORD);
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

struct t2fs_inode* read_i_node(int id_inode)
{
    struct t2fs_inode *inode = malloc(sizeof(*inode));

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
        inode->dataPtr[j] = (BYTE) buffer[inode_relative + i] + ((BYTE) buffer[inode_relative + i+1] << 8) +
                           ((BYTE) buffer[inode_relative + i+2] << 16) + ((BYTE) buffer[inode_relative + i+3] << 24);
    }
    //indireção simples
    i = 40;
    inode->singleIndPtr = (BYTE) buffer[inode_relative + i] + ((BYTE) buffer[inode_relative + i+1] << 8) +
                         ((BYTE) buffer[inode_relative + i+2] << 16) + ((BYTE) buffer[inode_relative + i+3] << 24);
    //indireção dupla
    i = 44;
    inode->doubleIndPtr = (BYTE) buffer[inode_relative + i] + ((BYTE) buffer[inode_relative + i+1] << 8) +
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
            printf("ERRO: Nome passado contem um caractere invalido");
            return 0;
        }
    }
    if (i > 30) {
        printf("ERRO: Nome passado e maior que 30 caracteres.");
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

    printf("Checando bitmap Blocos\n");
    for (i = 0; i < superBloco.NofBlocks; i++) {
        printf("bloco %d = %d, ", i, get_bitmap_state(i, BLOCK, superBloco));
    }

    set_on_bitmap(0, 0, BLOCK, superBloco);

    return 0;
}

/**
 *  Funcao que cria um novo arquivo no diretorio atual, dado o seu nome.
 */
FILE2 create2(char *filename)
{
    checkSuperBloco();

    //acha inode
    char *before_filename = get_string_before_last_bar(filename);
    struct t2fs_inode creating_inode;
    if (*before_filename != 0){ //se nome não tem barras
        find_inode_from_path(superBloco, filename, current_dir, &creating_inode);
    } else {
        creating_inode = current_dir;
    }

    char *striped_filename = get_string_after_bar(filename);

    if (!is_name_consistent(striped_filename)) {
        return -1;
    }

    struct t2fs_record *new_file_record;
    struct t2fs_inode *new_file_inode;

    int idx = get_free_bit_on_bitmap(INODE, superBloco);
    printf("Primeiro indice livre de inode e %d", idx);

    // TODO: No momento só guarda no diretório corrente,
    //       não permite passagem do caminho completo.

    // Cria o record para o arquivo
    new_file_record                 = malloc(sizeof *new_file_record);
    new_file_record->TypeVal        = TYPEVAL_REGULAR;
    new_file_record->i_node         = idx;
    new_file_record->blocksFileSize = 1;         // ocupa 1 inode quando criado
    new_file_record->bytesFileSize  = superBloco.BlockSize;        // TODO-: 31 bytes?? ou 0? --> tamanho do arquivo = tamanho bloco x blocs usados
    memcpy(new_file_record->name, striped_filename, 31);

    // Cria inode para arquivo
    new_file_inode = malloc(sizeof *new_file_inode);
    initialize_inode(new_file_inode);
    write_inode(idx, new_file_inode);
    free(new_file_inode);

    add_record_to_inode(creating_inode, *new_file_record);

    return open2(filename);
}

int delete2(char *filename)
{
    checkSuperBloco();
    struct t2fs_inode *inode = read_i_node(0);
    print_inode(*inode);
    // Escreve inode igual ao inode 0 no inode 1
    write_inode(1, inode);
    free(inode);

    struct t2fs_inode *inode1 = read_i_node(1);
    print_inode(*inode1);
    free(inode1);

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
    checkSuperBloco();
    return 0;
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
        free(searcher->next->inode);
        free(searcher->next);
        searcher = NULL;
        open_files->size--;
        printf("Arquivo %d fechado com sucesso\n", handle);
        return 0;
    }
    OPEN_FILE *aux = searcher->next;
    searcher->next = searcher->next->next;
    free(aux->inode);
    free(aux);
    open_files->size--;
    printf("Arquivo %d fechado com sucesso\n", handle);
    return 0;
}

int read2(FILE2 handle, char *buffer, int size)
{
    checkSuperBloco();
    return 0;
}

/**
 *  Dado o id de um bloco, retorna um array com os indices contidos nesse bloco.
 *  O array é alocado dinamicamente, entao é responsabilidade do chamador desalocar
 *  a memória.
 */
DWORD* get_indices(int id_block)
{
    DWORD *indices = malloc(get_num_indices() * sizeof(*indices));

    char buffer[superBloco.BlockSize];
    read_block(id_block, buffer, superBloco);

    unsigned int i;
    for (i = 0; i < get_num_indices(); i++) {
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
            printf("ERRO: inode nao contem bloco relativo de indice %d", relative_index);
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
    if (id_singleInd_block >= 0 && id_singleInd_block < get_num_indices()) { // Indice pertence aos blocos indiretos
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
    printf("JESUS CRISTO, AINDA NAO TA IMPLEMENTADO PRA ARQUIVOS TAO GRANDES");
    //int id = relative_index - 10 - N_OF_INDICES;
    //DWORD *first_lvl_id = get_indices(inode->doubleIndPtr);
    return -1;
}

/**
 *  Dado um índice de um bloco. Seta as entradas de todos os indices
 *  para 0x0FFFFFFFF. Sendo que o numero de indices e dado por (get_num_indices())
 */
void init_indices_block(int id_block)
{
    char buffer[superBloco.BlockSize];
    DWORD invalid = 0x0FFFFFFFF;

    int i;
    for (i = 0; i < get_num_indices(); i++) {
        memcpy(buffer+(i*sizeof(DWORD)), (char*)&invalid, sizeof(DWORD));
    }

    write_block(id_block, buffer, superBloco);
}

void print_indices(int id_block)
{
    DWORD *indices = get_indices(id_block);
    int i;
    printf("\n");
    for (i = 0; i < get_num_indices(); i++) {
        printf("id %d = %d\n", i, indices[i]);
    }
    free(indices);
}

int write_indices(int id_block, DWORD *indices)
{
    char buffer[superBloco.BlockSize];
    int i;
    for (i = 0; i < get_num_indices(); i++) {
        memcpy(buffer+(i*sizeof(DWORD)), indices+i, sizeof(DWORD));
    }

    write_block(id_block, buffer, superBloco);
    return 0;
}

int allocate_block_on_inode(struct t2fs_inode *inode)
{
    int new_id;
    printf("\nAlocando bloco ao inode\n");

    int i;
    for (i = 0; i < 10; i++) {
        if (inode->dataPtr[i] == INVALID_POINTER) {
            new_id = get_free_bit_on_bitmap(BLOCK, superBloco);
            inode->dataPtr[i] = new_id;
            set_on_bitmap(new_id, 1, BLOCK, superBloco);
            printf("\nBloco alocado no indice %d com o bloco %d\n", i, new_id);
            return 0;
        }
    }
    if (inode->singleIndPtr == INVALID_POINTER) { // Aloca bloco de indices
        printf("Alocando bloco de indices\n");

        new_id = get_free_bit_on_bitmap(BLOCK, superBloco);
        inode->singleIndPtr = new_id;

        init_indices_block(inode->singleIndPtr);
        set_on_bitmap(inode->singleIndPtr, 1, BLOCK, superBloco);

        DWORD *indices = get_indices(inode->singleIndPtr);
        indices[0] = get_free_bit_on_bitmap(BLOCK, superBloco);
        set_on_bitmap(indices[0], 1, BLOCK, superBloco);

        write_indices(inode->singleIndPtr, indices);
        free(indices);
        return 0;
    }
    // Se ja existe bloco de indices
    DWORD *indices = get_indices(inode->singleIndPtr);
    print_indices(inode->singleIndPtr);
    for (i = 0; i < get_num_indices(); i++) {
        if (indices[i] == INVALID_POINTER) {
            new_id = get_free_bit_on_bitmap(BLOCK, superBloco);
            indices[i] = new_id;

            set_on_bitmap(new_id, 1, BLOCK, superBloco);
            write_indices(inode->singleIndPtr, indices);
            free(indices);
            return 0;
        }
    }
    // Nenhum indice livre em single ind.
    printf("\nAGORA TEM QUE FAZER O DUPLO ENDERECO\n");
    if (inode->doubleIndPtr == INVALID_POINTER) {
        new_id = get_free_bit_on_bitmap(BLOCK, superBloco);
        inode->doubleIndPtr = new_id;
        set_on_bitmap(inode->doubleIndPtr, 1, BLOCK, superBloco);
        init_indices_block(inode->doubleIndPtr);
    }
    return 0;
}

/**
 *  Funcao que dado o handle do arquivo, escreve no mesmo o conteudo
 *  do buffer.
 */
int write2(FILE2 handle, char *buffer, int size)
{
    checkSuperBloco();

    int block_size     = superBloco.BlockSize;
    OPEN_FILE *file   = get_file_from_list(handle, FILE_TYPE);
    int position      = file->position;

    unsigned int first_block_id    = position / block_size;
    unsigned int blocks_to_read = ((position + size) / block_size) + 1;
    char to_write[blocks_to_read][block_size];

    unsigned int i;
    for (i = 0; i < blocks_to_read; i++) {
        int real_block_id = get_block_id_from_inode(first_block_id+i, file->inode);
        if (real_block_id == -1) { // Bloco ainda não está alocado ao arquivo
            // Seta indice do bloco para usado.
            if (get_bitmap_state(real_block_id, BLOCK, superBloco) == 0) {
                set_on_bitmap(real_block_id, 1, BLOCK, superBloco);
            } else {
                printf("ERRO: Tentando alocar bloco %d já sendo usado.", real_block_id);
                return -1;
            }
            if (allocate_block_on_inode(file->inode) == -1) {
                printf("ERRO: Falha ao tentar alocar bloco a arquivo.");
                return -1;
            }
            real_block_id = get_block_id_from_inode(first_block_id+i, file->inode);
        }
        read_block(real_block_id, &to_write[i][0], superBloco);
    }


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
    struct t2fs_inode *inode = read_i_node(1);
    initialize_inode(inode);
    int i;
    for (i = 0; i < 10; i++) {
        allocate_block_on_inode(inode);
        print_inode(*inode);
    }
    //print_indices(inode->singleIndPtr);
    for (i = 0; i < get_num_indices(); i++) {
        allocate_block_on_inode(inode);
        print_inode(*inode);
    }


    //print_inode(*inode);
    print_indices(inode->singleIndPtr);
    allocate_block_on_inode(inode);
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

    if ((open_files->size + open_directories->size) == MAX_OPEN_FILES) {
        printf("ERRO: numero maximo de arquivos abertos (20)");
        return -1;
    }
    if (pathname[0] == '/' && open_directories->first == NULL) { //diretório raíz e primeiro diretorio
        OPEN_FILE *open_file = malloc(sizeof *open_file);
        open_file->inode    = read_i_node(0);
        open_file->position = 0;
        open_file->handle   = current_handle;
        open_file->next     = NULL;
        add_opened_file_to_list(open_file, DIR_TYPE);

        current_handle++;
        return current_handle-1;
    }
    return -1;
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
    }else {
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

    read_records(workingFile->inode->dataPtr[0], records); //lê records apontados pelo primeiro ponteiro de I-node

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

int chdir2(char *pathname)
{
    checkSuperBloco();
    return find_inode_from_path(superBloco, pathname, current_dir, &current_dir);
}

int getcwd2(char *pathname, int size)
{
    checkSuperBloco();
    printf("current inode first block index: %u\n", current_dir.dataPtr[0]);
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

        current_dir = *read_i_node(0); //define diretório raíz como diretório atual
    }
}
