#include <stdio.h>
#include "./include/t2fs.h"

void ls(DIR2 dirHandle)
{
    DIRENT2 dirEntry;

    while(readdir2(dirHandle, &dirEntry) >= 0) {
        printf("Name: %s Type: %d Size: %lu\n", dirEntry.name, dirEntry.fileType, dirEntry.fileSize);
    }
}

int main(int argc, char *argv[])
{
    int handleArq = create2("Teste");
    char buffer[] = "O dia em que eu sai de casa minha mae me disse, meu filho vem caaaaaa....... passou as maos em meus cabelos olhou emmeu olhos,"
        "comecou a falar...... sou um garto pequeno com amor nos meus olhos, e fome de caminhar...sabado de sol alugie um caminhao para levar a gal,"
        "pra cioner feijao, chegano la ma que vergonhe so toinha maconhe, os maconheiro tava doidao querendo o meu feijao, uioasj, ahoaoajaoajaoaja"
        "We can go get drunk, staying up all night Singing Dont Stop Believing til the morning light We can line up shots til it all goes bad"
        "And were passed out and puking in a taxi cab When youre lying here by my side (Oh yeah!) Nowhere else feels right (Oh yeah!)"
        "Id rather be alone with you on a Saturday night, This is gonna be a night so epic (Saturday night, saturday night) You and be baby nothing but netflix"
        "(Saturday night, saturday night) We dont have to go, we can stay right here, We can go get drunk, staying up all night"
        "Singing Dont Stop Believing til the morning light We can line up shots til it all goes bad And were passed out and puking in a taxi cab"
        "When youre lying here by my side (Oh yeah!) Nowhere else feels right (Oh yeah!) Id rather be alone with you on a Saturday night\0";
    int size = 0;
    while (buffer[size] != '\0') {
        size++;
    }
    printf("BUFFER SIZE: %d\n", size+1);
    write2(handleArq, buffer, size+1);

    seek2(handleArq, -1);
    char buffer2[] = "LEONARDO";
    write2(handleArq, buffer2, 8);

    DIR2 handleDir = opendir2("/");

    char buffer_read2[size-1];
    seek2(handleArq, 0);
    int bytes_read = read2(handleArq, buffer_read2, size-1);
    printf("bytes read: %d\n", bytes_read);
    int i;
    for (i = 0; i < size; i++) {
        printf("%c", buffer_read2[i]);
    }

    /* char path[100]; */
    /* chdir2("/"); */
    /* DIR2 handler = mkdir2("/bla"); */
    /* chdir2("bla"); */
    /* DIR2 handler2 = mkdir2("/bla/blah"); */

    /* getcwd2(path, 100); */
    /* printf("%s\n", path); */

    /* chdir2("/bla/blah"); */

    /* getcwd2(path, 100); */
    /* printf("%s\n", path); */
    return 0;
}
