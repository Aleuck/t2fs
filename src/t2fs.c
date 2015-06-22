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


/***********************************/
/* Definicao do corpo das Funcoes **/
/***********************************/

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
        printf("%08xh; ", inode.dataPtr[i]);
    }
    printf("\n");

    printf("Indirecao simples: 0x%08x\n", inode.singleIndPtr);
    printf("Indirecao dupla: 0x%08x\n", inode.doubleIndPtr);

}

int write_inode(int id_inode, struct t2fs_inode *inode)
{
    int i;
    char buffer[superBloco.BlockSize];
    int first_inode = superBloco.InodeBlock;

    for (i = 0; i < 10; i++) {
        memcpy(buffer+(i*4), inode->dataPtr+i, 4);
    }
    memcpy(buffer+40, &inode->singleIndPtr, 4);
    memcpy(buffer+44, &inode->doubleIndPtr, 4);

    write_block(first_inode + id_inode, buffer, superBloco);
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
 *  Funcao que retorna o indice do primeiro elemento livre no
 *  bitmap indicado por *type*.
 */
int get_free_bit_on_bitmap(bitmap_type type)
{
    int idx = 0;
    while (get_bitmap_state(idx, type, superBloco) != 0) {
        idx++;
    }
    return idx;
}

/**
 *  Funcao que seta todas entradas do inode para 0x0FFFFFFFF
 */
void initialize_inode(struct t2fs_inode *inode)
{
    int i;
    for (i = 0; i < 10; i++) {
        inode->dataPtr[i] = 0x0FFFFFFFF;
    }
    inode->singleIndPtr = 0x0FFFFFFFF;
    inode->doubleIndPtr = 0x0FFFFFFFF;
}

/************************************************/
/* Definicao das funcoes principais do trabalho */
/************************************************/

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

    printf("Checando bitmap Blocos\n");
    for (i = 0; i < superBloco.NofBlocks; i++) {
        printf("bloco %d = %d, ", i, get_bitmap_state(i, BLOCK, superBloco));
    }

    set_on_bitmap(0, 0, BLOCK, superBloco);

    return 0;
}

/**
 *  Funcao que cria um novo arquivo, dado o seu nome.
 */
FILE2 create2(char *filename)
{
    checkSuperBloco();

    if (!is_name_consistent(filename)) {
        return -1;
    }

    struct t2fs_record *new_file;
    struct t2fs_inode *new_file_inode;
    struct t2fs_inode *dir_inode;
    OPEN_FILE *current_dir;

    int idx = get_free_bit_on_bitmap(INODE);
    printf("Primeiro indice livre de inode e %d", idx);

    // TODO: No momento assuma que diretorio atual e o primeiro da lista. Necessario Arrumar
    current_dir = open_directories->first;
    dir_inode = current_dir->inode;

    // Cria o record para o arquivo
    new_file                 = malloc(sizeof *new_file);
    new_file->TypeVal        = TYPEVAL_REGULAR;
    new_file->i_node         = idx;
    new_file->blocksFileSize = 1;         // ocupa 1 inode quando criado
    new_file->bytesFileSize  = 31;        // TODO: 31 bytes?? ou 0? (31 bytes tem o nome)
    strncpy(new_file->name, filename, 31);
    // Cria inode para arquivo
    new_file_inode = malloc(sizeof *new_file_inode);
    initialize_inode(new_file_inode);
    write_inode(idx, new_file_inode);
    // TODO: Adicionar Record no Arquivo.
    // ...
    // ...
    return 0;
}

int delete2(char *filename)
{
    checkSuperBloco();
    print_bitmap(BLOCK, superBloco);
    printf("\nINODES\n");
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
