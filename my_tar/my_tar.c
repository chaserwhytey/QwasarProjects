#include "my_tar.h"
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <dirent.h>
#include <utime.h>

int tar(char** av, int fFlag, char* tarName, int* i, char* path, int mode, int ac) {
    char buffer[100] = {0};
    if(fFlag) {
        my_strncpy(&buffer[my_strlen(buffer)], av[*i], my_strlen(av[*i]) + 1);
        *i = *i + 1;
        tarName = buffer;
    }
    switch(mode) {
        case cMode: 
            createTar(tarName, av, path, fFlag, i, ac);
            break;
        case tMode:
            if (*i < ac - 1) {
                printf("error: only one archive file can be specified for listing\n");
                return 1;
            }
            if(!fFlag) {
                printf("error: archive -f must be specified\n");
                return 1;
            }
            if(tarList(tarName)) return 1;
            break;
        case xMode: 
            if(!fFlag) {
                printf("error: archive -f must be specified\n");
                return 1;
            }
            if(tarExtract(tarName, path)) return 1;
            break;
        case rMode:
            if(!fFlag) {
                printf("error: archive -f must be specified\n");
                return 1;
            }
            if(tarAppend(tarName, av, i, ac)) return 1;
            break;
        case uMode:
            if(!fFlag) {
                printf("error: archive -f must be specified\n");
                return 1;
            }
            if(updateEntry(tarName, av, *i, ac)) return 1;
            *i = ac;
            break;
        default: 
            printf("error: no mode specified\n");
            return 1;
            break;
    }
    return 0;
}

int handleOptions(char** av, char* path, int* i, int* fFlag, int* mode) {
    if((*av[*i] == '-') && *(av[*i] + 1) == 'C') {
        if( !(av[*i+1]) ) {
            printf("error: directory not specified\n");
            return 1;
        }
        if(!isDirectory(av[*i + 1])) {
            printf("error: directory %s does not exist", av[*i + 1]);
            return 1;
        }
        my_strncpy(path, av[*i + 1], my_strlen(av[*i + 1]) + 1);
        path[my_strlen(path)] = '/';
        *i = *i + 1;
    }
    else if(setFlags(mode, fFlag, av[*i] + 1)) {
        return 1;
    }
    return 0;
}

int setFlags(int* mode, int* fFlag, char* flags) {
    while(*flags) {
        if(*mode && *flags != 'f') {
            printf("error: only one mode may be specified\n");
            return 1;
        }
        switch(*flags++) {
            case 'c': 
                *mode = cMode;
                break;
            case 'r':
                *mode = rMode;
                break;
            case 't':
                *mode = tMode;
                break;
            case 'u':
                *mode = uMode;
                break;
            case 'x':
                *mode = xMode;
                break;
            case 'f':
                *fFlag = 1;
                break;
            case 'C': 
                printf("C must be specified seperately\n");
                break;
            default:
                printf("%c is not a valid option\n", *flags);
                return 1;
                break;
        }
    }
    return 0;
}

void setToZero(tar_header* head) {
    for(unsigned int i = 0; i < sizeof(head->name); i++) head->name[i] = '\0';
    for(unsigned int i = 0; i < sizeof(head->mode); i++) head->mode[i] = '\0';
    for(unsigned int i = 0; i < sizeof(head->owner); i++) head->owner[i] = '\0';
    for(unsigned int i = 0; i < sizeof(head->group); i++) head->group[i] = '\0';
    for(unsigned i = 0; i < sizeof(head->size); i++) head->size[i] = '\0';
    for(unsigned i = 0; i < sizeof(head->modified); i++) head->modified[i] = '\0';
    head->type = '\0';
    head->linkNum = '\0';
    for(unsigned i = 0; i < sizeof(head->link); i++) head->link[i] = '\0';
    for(unsigned i = 0; i < sizeof(head->padding); i++) head->padding[i] = '\0';
}

void createFile(char* fullPath, int flags, unsigned int mode, unsigned int size, unsigned int modTime, int type, int fd) {
    if(type == DIRECTORY) {
        mkdir(fullPath, mode);
    }
    else {
        char buffer[4096];
        int readFile = open(fullPath, flags, mode);
        writeToOffset(fd, readFile, size, buffer);
        int offset = size % BLOCK_SIZE == 0 ? 0 : BLOCK_SIZE - size % BLOCK_SIZE;
        lseek(fd, offset, SEEK_CUR);
        close(readFile);
    }
    struct utimbuf* times = (struct utimbuf*)malloc(sizeof(struct utimbuf));
    times->actime = modTime;
    times->modtime = modTime;
    utime(fullPath, times);
    free(times);
}

int tarExtract(char* tarName, char* path) {
    int fd = open(tarName, O_RDONLY);
    if(fd == -1) {
        printf("error: could not open file %s", tarName);
        return 1;
    }
    tar_header head;
    char fullPath[my_strlen(path) + sizeof(head.name) + 1];
    my_strncpy(fullPath, path, my_strlen(path));
    while(read(fd, &head, sizeof(tar_header))) {
        struct stat fileStat;
        unsigned long size = oct2num(head.size, 8, sizeof(head.size));
        unsigned long modTime = oct2num(head.modified, 8, sizeof(head.modified));
        unsigned int mode = oct2num(head.mode, 8, sizeof(head.mode));
        my_strncpy(fullPath + my_strlen(path), head.name, sizeof(head.name));
        int flags = (O_CREAT | O_WRONLY);
        if(!stat(fullPath, &fileStat)) {
            unsigned long time = fileStat.st_mtime;
            if(modTime > time) {
                flags |= O_TRUNC;
                createFile(fullPath, flags, mode, size, modTime, head.type, fd);
            }
            else if (head.type != DIRECTORY) {
                lseek(fd, size + BLOCK_SIZE - (size % BLOCK_SIZE == 0 ? BLOCK_SIZE : size % BLOCK_SIZE), SEEK_CUR);
            }
        }
        else {
            createFile(fullPath, flags, mode, size, modTime, head.type, fd);
        }
    }
    return 0;
}

int tarList(char* tarName) {
    int fd = open(tarName, O_RDONLY);
    if(fd < 0) {
        printf("error: archive %s does not exist\n", tarName);
        return 1;
    }
    tar_header head;
    while(read(fd, &head, sizeof(head))) {    
        printf("%s\n", head.name);
        if(head.type != DIRECTORY) {
            unsigned long size = oct2num(head.size, 8, sizeof(head.size));
            lseek(fd, size + (size % BLOCK_SIZE == 0 ? 0 : BLOCK_SIZE - size % BLOCK_SIZE), SEEK_CUR);
        }
    }
    close(fd);
    return 0;
}

int tarAppend(char* tarName, char** args, int* i, int numArgs) {
    int fd = open(tarName, O_RDWR, 0644);
    if(fd < 0) {
        printf("error: %s does not exist", tarName);
        return 1;
    }
    lseek(fd, 0, SEEK_END);
    for(; *i < numArgs; *i = *i + 1) {
        if(tarAdd(fd, args[*i])) return 1;
    }
    close(fd);
    return 0;
}

void readContents(char** dirFiles, tar_header* head) {
    struct dirent *de;  
    DIR *dp;
    dp = opendir(head->name);
    int index = 0;
    while((de = readdir(dp))) {
        if(de->d_name[0] != '.') {
            dirFiles[index] = (char*)malloc(100);
            my_strncpy(dirFiles[index], head->name, my_strlen(head->name) + 1);
            dirFiles[index][my_strlen(head->name)] = '/';
            my_strncpy(dirFiles[index++] + my_strlen(head->name) + 1, de->d_name, my_strlen(de->d_name) + 1);
        }
    }
    closedir(dp);
}

int updateDir(int fd, tar_header* head, struct stat fileStat, char* arg, char* tarName) {
    lseek(fd, -BLOCK_SIZE, SEEK_CUR);
    tar_header newDir;
    writeEntry(&newDir, fileStat, arg, fd);
    int fileCount = getFileCount(head->name);
    if(fileCount == -1) return 1;
    char** dirFiles = (char**)malloc(fileCount*sizeof(char*));
    readContents(dirFiles, head);
    int returnToOffset = lseek(fd, 0, SEEK_CUR);
    close(fd);
    if(updateEntry(tarName, dirFiles, 0, fileCount)) return 1; 
    freeStrings(dirFiles, fileCount);
    fd = open(tarName, O_RDWR);
    lseek(fd, returnToOffset, SEEK_SET);
    return 0;
}

void transferContents(int readFile, int tmp, char* buffer) {
    int bytes;
    for(unsigned int k = 0; k < sizeof(buffer); k++) buffer[k] = '\0';
    while((bytes = read(readFile, buffer, sizeof(buffer)))) {
        write(tmp, buffer, bytes);
    }
}

void writeToOffset(int fd, int tmp, unsigned int writeToVal, char* buffer) {
    int bytes;
    unsigned int totalBytes = 0;
    while(totalBytes != writeToVal) {
        bytes = read(fd, buffer, totalBytes + 4096 < writeToVal ? sizeof(buffer) : writeToVal - totalBytes);
        write(tmp, buffer, bytes);
        totalBytes += bytes;
    }
}

void insertEntry(int fd, unsigned long size, struct stat fileStat, char* arg) {
    unsigned int writeToVal = lseek(fd, 0, SEEK_CUR) - 512;
    unsigned int writeFromVal = lseek(fd, size, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    int tmp = open(tempFile, O_CREAT|O_RDWR|O_TRUNC, 0777);
    char buffer[4096];
    writeToOffset(fd, tmp, writeToVal, buffer);
    tar_header newEntry;
    writeEntry(&newEntry, fileStat, arg, tmp);
    unsigned long newSize = oct2num(newEntry.size, 8, sizeof(newEntry.size));
    unsigned long newOffset =  (newSize % 512 == 0 ? 0 : BLOCK_SIZE - newSize % BLOCK_SIZE);
    int readFile = open(arg, O_RDONLY);
    transferContents(readFile, tmp, buffer);
    close(readFile);
    int returnToOffset = lseek(tmp, newOffset, SEEK_CUR);
    lseek(fd, writeFromVal, SEEK_SET);
    transferContents(fd, tmp, buffer);
    lseek(tmp, 0, SEEK_SET);
    lseek(fd, 0, SEEK_SET);
    transferContents(tmp, fd, buffer);
    close(tmp);
    lseek(fd, returnToOffset, SEEK_SET);
}

int updateEntry(char *tarName, char **args, int i, int numArgs) {
    char contains[numArgs - i];
    for(int i = 0; i < (int) sizeof(contains); i++) {
        contains[i] = '0';
        
    }
    int fd = open(tarName, O_RDWR);
    if(fd < 0) {
        printf("error: %s does not exist", tarName);
        return 1;
    }
    tar_header head;
    while((read(fd, &head, sizeof(tar_header)))) {
        long unsigned size = oct2num(head.size, 8, sizeof(head.size));
        long unsigned offset =  size + (size % BLOCK_SIZE == 0 ? 0 : BLOCK_SIZE - size % BLOCK_SIZE);
        for(int j = i; j < numArgs; j++) {
            if(!my_strncmp(args[j], head.name, my_strlen(args[j]) > my_strlen(head.name) ? my_strlen(args[j]) : my_strlen(head.name))) {
                struct stat fileStat;
                if(stat(args[j], &fileStat) < 0) {   
                    printf("path name %s does not exist", args[j]); 
                    return 1;
                }
                if((unsigned long) fileStat.st_mtime > oct2num(head.modified, 8, sizeof(head.modified))) {
                    if(head.type == DIRECTORY) {
                        if(updateDir(fd, &head, fileStat, args[j], tarName)) return 1;
                    }
                    else {
                        insertEntry(fd, offset, fileStat, args[j]);
                        offset = 0;
                    }
                }
                contains[j - i] = '1';
            }
        }
        if(head.type != DIRECTORY) {
            lseek(fd, offset, SEEK_CUR);
        }
    }
    lseek(fd, 0, SEEK_END);
    for(int j = i; j < numArgs; j++) {
        if(contains[j - i] == '0') {
            tarAdd(fd, args[j]);
        }
    }
    close(fd);
    return 0;
}

int writeEntry(tar_header * head, struct stat fileStat, char* arg, int fd) {
    setToZero(head);
    my_strncpy(head->name, arg, my_strlen(arg));
    utoa(fileStat.st_mode & 0777, 8, head->mode);
    utoa(fileStat.st_uid, 8, head->owner);
    utoa(fileStat.st_gid, 8, head->group);
    utoa(fileStat.st_size, 8, head->size); 
    utoa(fileStat.st_mtime, 8, head->modified);
    switch(fileStat.st_mode & S_IFMT) {
        case S_IFREG: head->type = REGULAR; break;
        case S_IFDIR: head->type = DIRECTORY; break;
        case S_IFCHR: head->type = CHAR; break;
        case S_IFBLK: head->type = BLOCK; break;
        case S_IFIFO: head->type = FIFO; break;
        case S_IFSOCK: printf("error: cannot tar socket"); return 1; break;
        default: printf("error: unknown file type"); return 1; break;
    }
    utoa(fileStat.st_nlink, 10, &head->linkNum);
    write(fd, head, sizeof(tar_header));
    return 0;
}

void freeStrings(char** strings, int count) {
    for(int i = 0; i < count; i++) {
        free(strings[i]);
    }
    free(strings);
}

int tarAdd(int fd, char* arg) {
    struct stat fileStat;
    if(stat(arg, &fileStat) < 0) {   
        printf("path name %s does not exist", arg); 
        return 1;
    }
    tar_header head;
    if(writeEntry(&head, fileStat, arg, fd)) return 1;
    if(head.type == DIRECTORY) {
        int fileCount = getFileCount(head.name);
        char** dirFiles = (char**)malloc(fileCount*sizeof(char*));
        readContents(dirFiles, &head);
        for(int i = 0; i < fileCount; i++) {
            tarAdd(fd, dirFiles[i]);
        }
        freeStrings(dirFiles, fileCount);
    }
    else {
        int readFile = open(arg, O_RDONLY);
        unsigned long fileSize = fileStat.st_size;
        char buffer[4096];
        transferContents(readFile, fd, buffer);
        close(readFile);
        for(unsigned long i = 0; i < (fileSize % BLOCK_SIZE == 0 ? 0 : BLOCK_SIZE - fileSize % BLOCK_SIZE); i++) {
            write(fd, "\0", 1);
        }
    }
    return 0;
}

int createTar(char* tarName, char** args, char* path, int flag, int* i, int numArgs) {
    char fullPath[200];
    my_strncpy(fullPath, path, my_strlen(path));
    if(tarName)
        my_strncpy(fullPath + my_strlen(path), tarName, my_strlen(tarName) + 1);
    int fd = flag ? open(fullPath, O_CREAT|O_RDWR|O_TRUNC, 0644) : 1;
    for(; *i < numArgs; *i = *i + 1) {
        if(tarAdd(fd, args[*i])) return 1;
    }
    close(fd);
    return 0;
}
