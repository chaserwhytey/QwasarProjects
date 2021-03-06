#ifndef MY_TAR_H
#define MY_TAR_H
#include <sys/stat.h>

#define BLOCK_SIZE 512
#define nums "0123456789"
#define tempFile "/tmp/ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%!@#$%!@#"

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

int tar(char** av, int fFlag, char* tarName, int* i, char* path, int mode, int ac);
int handleOptions(char** av, char* path, int* i, int* fFlag, int* mode);
int isDirectory(const char *path);
int my_strncmp(char* s1, char* s2, int length);
int my_strlen(char* c1);
void my_strncpy(char* c1, char* c2, int length);
int getFileCount(char* path);
void utoa(unsigned int val, int base, char* var);
unsigned long oct2num(char* oct, int base, int size);
int setFlags(int* mode, int* fFlag, char* flags);
int extractTarEntry(tar_header** head, int fd);
void setToZero(tar_header* head);
void creatFile(char* fullPath, int flags, unsigned int mode, unsigned long size, unsigned long modTime, int type, int fd);
int tarExtract(char* tarName, char* path);
int tarList(char* tarName);
int tarAppend(char* tarName, char** args, int* i, int numArgs);
void readContents(char** dirFiles, tar_header* head);
int updateDir(int fd, tar_header* head, struct stat fileStat, char* arg, char* tarName);
void transferContents(int readFile, int tmp, char* buffer);
void writeToOffset(int fd, int tmp, unsigned int writeToVal, char* buffer);
void insertEntry(int fd, unsigned long size, struct stat fileStat, char* arg);
int updateEntry(char* tarName, char** args, int i, int numArgs);
int writeEntry(tar_header * head, struct stat fileStat, char* arg, int fd);
void freeStrings(char** strings, int count);
int tarAdd(int fd, char* argument);
int createTar(char* tarName, char** args, char* path, int flag, int* i, int numArgs);

#endif
