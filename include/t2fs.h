#ifndef __LIBT2FS___
#define __LIBT2FS___

#define TYPEVAL_REGULAR     0x01
#define TYPEVAL_DIRETORIO   0x02
#define TYPEVAL_INVALIDO    0xFF

typedef int FILE2;
typedef int DIR2;

typedef unsigned char BYTE;
typedef unsigned short int WORD;
typedef unsigned int DWORD;

/** Superbloco */
struct t2fs_superbloco {
    char    Id[4];          /* Identificação do sistema de arquivo. É formado pelas letras “T2FS”. */
    WORD    Version;        /* Versão atual desse sistema de arquivos: (valor fixo 7DE=2014; 2=2 semestre). */
    WORD    SuperBlockSize; /* Quantidade de setores  que formam o superbloco. (fixo em 1 setor) */
    DWORD   DiskSize;       /* Tamanho total da partição T2FS, incluindo o tamanho do superblock. */
    DWORD   NofBlocks;      /* Quantidade total de blocos de dados na partição T2FS (1024 blocos). */
    DWORD   BlockSize;      /* Tamanho de um bloco.*/
    DWORD   BitmapBlocks;   /* Primeiro bloco lógico do bitmap de blocos livres e ocupados*/
    DWORD   BitmapInodes;   /* Primeiro bloco lógico do bitmap de i-nodes livres e ocupados.*/
    DWORD   InodeBlock;     /* Primeiro bloco lógico da área de i-nodes*/
    DWORD   FirstDataBlock; /* Primeiro bloco lógico da área de blocos de dados.*/
} __attribute__((packed));

/** Registro de diretório (entrada de diretório) */
struct t2fs_record {
    BYTE    TypeVal;        /* Tipo da entrada. Indica se o registro é inválido (0xFF), arquivo (0x01 ou diretório (0x02) */
    char    name[31];       /* Nome do arquivo. : string com caracteres ASCII (0x21 até 0x7A), case sensitive. */
    DWORD   blocksFileSize; /* Tamanho do arquivo, expresso em número de blocos de dados */
    DWORD   bytesFileSize;  /* Tamanho do arquivo. Expresso em número de bytes. */
    DWORD   i_node;         /* i-node do arquivo */
    char    Reserved[20];
} __attribute__((packed));

/** i-node */
struct t2fs_inode {
    DWORD   dataPtr[10];    /* Ponteiros diretos para blocos de dados do arquivo */
    DWORD   singleIndPtr;   /* Ponteiro de indireção simples */
    DWORD   doubleIndPtr;   /* Ponteiro de indireção dupla */
    char    Reserved[16];   /* Não usados */
} __attribute__((packed));

#define MAX_FILE_NAME_SIZE 255
typedef struct {
    char name[MAX_FILE_NAME_SIZE+1];
    int fileType;   // ==1, is directory; ==0 is file
    unsigned long fileSize;
} DIRENT2;

int identify2 (char *name, int size);

FILE2 create2 (char *filename);
int delete2 (char *filename);
FILE2 open2 (char *filename);
int close2 (FILE2 handle);
int read2 (FILE2 handle, char *buffer, int size);
int write2 (FILE2 handle, char *buffer, int size);
int seek2 (FILE2 handle, unsigned int offset);

int mkdir2 (char *pathname);
int rmdir2 (char *pathname);

DIR2 opendir2 (char *pathname);
int readdir2 (DIR2 handle, DIRENT2 *dentry);
int closedir2 (DIR2 handle);

int chdir2 (char *pathname);
int getcwd2 (char *pathname, int size);


struct t2fs_inode read_i_node(int id_inode);
int get_records_in_block();
void read_records(DWORD id_block, struct t2fs_record records[]);

#endif
