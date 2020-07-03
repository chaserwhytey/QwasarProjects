#ifndef MY_TAR_H
#define MY_TAR_H

#include<string.h>

#define BLOCK_SIZE 512
#define nums "0123456789"

#define REGULAR         '1'
#define SYMLINK         '2'
#define CHAR            '3'
#define BLOCK           '4'
#define DIRECTORY       '5'
#define FIFO            '6'
#define CONTIGUOUS      '7'

#define cMode 1
#define rMode 2
#define tMode 3
#define uMode 4
#define xMode 5

typedef struct tar_header{
    char name[100];
    char mode[8];
    char owner[8];
    char group[8];
    char size[12];
    char modified[12];
    char type;
    char linkNum;
    char link[100];
    char padding[BLOCK_SIZE - 100 - 8 - 8 - 8 - 12 - 12 - 1 - 1 - 100];
} tar_header;

void utoa(unsigned int val, int base, int size, int fd);
unsigned int oct2num(char* oct, int base, int size);
int setFlags(int* mode, int* fFlag, char* flags);
int extractTarEntry(tar_header** head, int fd);
int tarExtract(char* tarName, char* path);
void tarList(char* tarName);
int tarAdd(int fd, char* argument, char* path, int* pos);
int createTar(char* tarName, char** args, char* path, int flag, int* i, int numArgs);

#endif