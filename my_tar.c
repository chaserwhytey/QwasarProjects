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

int extractTarEntry(tar_header** head, int fd) {
    if(!read(fd, (*head)->name, sizeof((*head)->name))) return 0;
    read(fd, (*head)->mode, sizeof((*head)->mode));
    read(fd, (*head)->owner, sizeof((*head)->owner));
    read(fd, (*head)->group, sizeof((*head)->group));
    read(fd, (*head)->size, sizeof((*head)->size));
    read(fd, (*head)->modified, sizeof((*head)->modified));
    read(fd, &(*head)->type, sizeof((*head)->type));
    read(fd, &(*head)->linkNum, sizeof((*head)->linkNum));
    read(fd, (*head)->link, sizeof((*head)->link));
    lseek(fd, sizeof((*head)->padding), SEEK_CUR);
    return 1;
}

void createFile(char* fullPath, int flags, unsigned int mode, unsigned int size, unsigned int modTime, int type, int fd) {
    if(type == DIRECTORY) {
        mkdir(fullPath, mode);
    }
    else {
        char buffer[size];
        read(fd, buffer, size);
        int offset = size % BLOCK_SIZE;
        if(offset != 0) {
            offset = BLOCK_SIZE - offset;
        }
        lseek(fd, offset, SEEK_CUR);
        int readFile = open(fullPath, flags, mode);
        write(readFile, buffer, size);
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
    tar_header* head = (tar_header*)malloc(sizeof(tar_header));
    char fullPath[my_strlen(path) + sizeof(head->name) + 1];
    my_strncpy(fullPath, path, my_strlen(path));
    while(extractTarEntry(&head, fd)) {
        struct stat fileStat;
        unsigned long size = oct2num(head->size, 8, sizeof(head->size));
        unsigned long modTime = oct2num(head->modified, 8, sizeof(head->modified));
        unsigned int mode = oct2num(head->mode, 8, sizeof(head->mode));
        my_strncpy(fullPath + my_strlen(path), head->name, sizeof(head->name));
        int flags = (O_CREAT | O_WRONLY);
        if(!stat(fullPath, &fileStat)) {
            unsigned long time = fileStat.st_mtime;
            if(modTime > time) {
                flags |= O_TRUNC;
                createFile(fullPath, flags, mode, size, modTime, head->type, fd);
            }
            else if (head->type != DIRECTORY) {
                lseek(fd, size + BLOCK_SIZE - (size % BLOCK_SIZE == 0 ? BLOCK_SIZE : size % BLOCK_SIZE), SEEK_CUR);
            }
        }
        else {
            createFile(fullPath, flags, mode, size, modTime, head->type, fd);
        }
        free(head);
        head = (tar_header*)malloc(sizeof(tar_header));
    }
    free(head);
    return 0;
}

int tarList(char* tarName) {
    int fd = open(tarName, O_RDONLY);
    if(fd < 0) {
        printf("error: archive %s does not exist\n", tarName);
        return 1;
    }
    tar_header* head = (tar_header*)malloc(sizeof(tar_header));
    while(extractTarEntry(&head, fd)) {    
        printf("%s\n", head->name);
        if(head->type != DIRECTORY) {
            unsigned long size = oct2num(head->size, 8, sizeof(head->size));
            lseek(fd, size + BLOCK_SIZE - (size % BLOCK_SIZE == 0 ? BLOCK_SIZE : size % BLOCK_SIZE), SEEK_CUR);
        }
        free(head);
        head = (tar_header*)malloc(sizeof(tar_header));
    }
    free(head);
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

int insertEntry(int fd, unsigned long size, struct stat fileStat, char* arg) {
    unsigned int writeToVal = lseek(fd, 0, SEEK_CUR) - 512;
    unsigned int writeFromVal = lseek(fd, size + (size % 512 == 0 ? 0 : BLOCK_SIZE - size % BLOCK_SIZE), SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    int tmp = open(tempFile, O_CREAT|O_RDWR|O_TRUNC, 0777);
    char buffer[4096] = { 0 };
    int bytes;
    unsigned int totalBytes = 0;
    while(totalBytes != writeToVal) {
        bytes = read(fd, buffer, totalBytes + 4096 < writeToVal ? sizeof(buffer) : writeToVal - totalBytes);
        write(tmp, buffer, bytes);
        totalBytes += bytes;
    }
    tar_header* newEntry = (tar_header*)malloc(sizeof(tar_header));
    writeEntry(&newEntry, fileStat, arg, tmp);
    long unsigned newSize = oct2num(newEntry->size, 8, sizeof(newEntry->size));
    int readFile = open(arg, O_RDONLY);
    for(int k = 0; k < 4096; k++) buffer[k] = '\0';
    while((bytes = read(readFile, buffer, sizeof(buffer)))) {
        write(tmp, buffer, bytes);
    }
    close(readFile);
    int offset = lseek(tmp, newSize % BLOCK_SIZE == 0 ? 0 : BLOCK_SIZE - newSize % BLOCK_SIZE, SEEK_CUR);
    lseek(fd, writeFromVal, SEEK_SET);
    for(int k = 0; k < 4096; k++) buffer[k] = '\0';
    while((bytes = read(fd, buffer, sizeof(buffer)))) {
        write(tmp, buffer, bytes);
    }
    lseek(tmp, 0, SEEK_SET);
    lseek(fd, 0, SEEK_SET);
    for(int k = 0; k < 4096; k++) buffer[k] = '\0';
    while((bytes = read(tmp, buffer, sizeof(buffer)))) {
        write(fd, buffer, bytes);
    }
    close(tmp);
    free(newEntry);
    return offset;
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
    tar_header* head = (tar_header*)malloc(sizeof(tar_header));
    while((extractTarEntry(&head, fd))) {
        long unsigned size = oct2num(head->size, 8, sizeof(head->size));
        int offset = size + BLOCK_SIZE - (size % BLOCK_SIZE == 0 ? BLOCK_SIZE : size % BLOCK_SIZE);
        for(int j = i; j < numArgs; j++) {
            if(!my_strncmp(args[j], head->name, my_strlen(args[j]) > my_strlen(head->name) ? my_strlen(args[j]) : my_strlen(head->name))) {
                struct stat fileStat;
                if(stat(args[j], &fileStat) < 0) {   
                    printf("path name %s does not exist", args[j]); 
                    return 1;
                }
                if((unsigned long) fileStat.st_mtime > oct2num(head->modified, 8, sizeof(head->modified))) {
                    if(head->type == DIRECTORY) {
                        lseek(fd, -BLOCK_SIZE, SEEK_CUR);
                        tar_header* newDir = (tar_header*)malloc(sizeof(tar_header));
                        writeEntry(&newDir, fileStat, args[j], fd);
                        int fileCount = getFileCount(head->name);
                        if(fileCount == -1) return 1;
                        struct dirent *de;  
                        DIR *dp;
                        dp = opendir(head->name);
                        char** dirFiles = (char**)malloc(fileCount*sizeof(char*));
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
                        int returnToOffset = lseek(fd, 0, SEEK_CUR);
                        close(fd);
                        if(updateEntry(tarName, dirFiles, 0, fileCount)) return 1; 
                        for(int k = 0; k < fileCount; k++) {
                            free(dirFiles[k]);
                        }
                        free(dirFiles);
                        free(newDir);
                        fd = open(tarName, O_RDWR);
                        lseek(fd, returnToOffset, SEEK_SET);
                    }
                    else {
                        offset = insertEntry(fd, size, fileStat, args[j]);
                    }
                }
                contains[j - i] = '1';
            }
        }
        if(head->type != DIRECTORY) {
            lseek(fd, offset, SEEK_CUR);
        }
        free(head);
        head = (tar_header*)malloc(sizeof(tar_header));
    }
    free(head);
    lseek(fd, 0, SEEK_END);
    for(int j = i; j < numArgs; j++) {
        if(contains[j - i] == '0') {
            tarAdd(fd, args[j]);
        }
    }
    
    close(fd);
    return 0;
}

int writeEntry(tar_header ** head, struct stat fileStat, char* arg, int fd) {
    my_strncpy((*head)->name, arg, my_strlen(arg) + 1);
    write(fd, arg, my_strlen(arg) + 1);
    lseek(fd, sizeof((*head)->name) - (my_strlen(arg) + 1), SEEK_CUR);
    utoa(fileStat.st_mode & 0777, 8, sizeof((*head)->mode), fd, (*head)->mode);
    utoa(fileStat.st_uid, 8, sizeof((*head)->owner), fd, (*head)->owner);
    utoa(fileStat.st_gid, 8, sizeof((*head)->group), fd, (*head)->group);
    utoa(fileStat.st_size, 8, sizeof((*head)->size), fd, (*head)->size); 
    utoa(fileStat.st_mtime, 8, sizeof((*head)->modified), fd, (*head)->modified);
    
    switch(fileStat.st_mode & S_IFMT) {
        case S_IFREG: (*head)->type = REGULAR; break;
        case S_IFDIR: (*head)->type = DIRECTORY; break;
        case S_IFCHR: (*head)->type = CHAR; break;
        case S_IFBLK: (*head)->type = BLOCK; break;
        case S_IFIFO: (*head)->type = FIFO; break;
        case S_IFSOCK: printf("error: cannot tar socket"); return 1; break;
        default: printf("error: unknown file type"); return 1; break;
    }

    write(fd, &(*head)->type, 1);
    utoa(fileStat.st_nlink, 10, sizeof((*head)->linkNum), fd, &(*head)->linkNum);

    errno = 0;
    ssize_t bufSize = readlink(arg, (*head)->link, 100);
    if(errno != 0 && errno != EINVAL) {
        printf("error: reading link");
        return 1;
    }
    if(bufSize == 100) {
        printf("error: sym link path too long");
    }
    if(bufSize < 100 && bufSize > 0) {
        (*head)->type = SYMLINK;
        (*head)->link[my_strlen((*head)->link)] = '\0';
        write(fd, (*head)->link, bufSize);
        lseek(fd, sizeof((*head)->link) - bufSize, SEEK_CUR);
    }
    else {
        lseek(fd, sizeof((*head)->link), SEEK_CUR);
    }
    lseek(fd, sizeof((*head)->padding), SEEK_CUR);
    return 0;
}

int tarAdd(int fd, char* arg) {
    struct stat fileStat;
    if(stat(arg, &fileStat) < 0) {   
        printf("path name %s does not exist", arg); 
        return 1;
    }
    tar_header* head = (tar_header*)malloc(sizeof(tar_header));
    if(writeEntry(&head, fileStat, arg, fd)) return 1;
    if(head->type == DIRECTORY) {
        struct dirent *de;  
        DIR *dp;
        dp = opendir(arg);
        char buffer[200];
        my_strncpy(buffer, arg, my_strlen(arg) + 1);
        buffer[my_strlen(head->name)] = '/';
        while((de = readdir(dp))) {
            my_strncpy(buffer + my_strlen(head->name) + 1, de->d_name, my_strlen(de->d_name) + 1);
            if(de->d_name[0] != '.') {
                tarAdd(fd, buffer);
            }
        }
        closedir(dp);
    }
    else {
        int readFile = open(arg, O_RDONLY);
        unsigned long fileSize = fileStat.st_size;
        char buffer[4096];
        int bytes;
        while((bytes = read(readFile, buffer, sizeof(buffer)))) {
            write(fd, buffer, bytes);
        }
        close(readFile);
        for(unsigned long i = 0; i < BLOCK_SIZE - (fileSize % BLOCK_SIZE == 0 ? BLOCK_SIZE : fileSize % BLOCK_SIZE); i++) {
            write(fd, "\0", 1);
        }
    }
    free(head);
    return 0;
}

int createTar(char* tarName, char** args, char* path, int flag, int* i, int numArgs) {
    char fullPath[200];
    my_strncpy(fullPath, path, my_strlen(path));
    my_strncpy(fullPath + my_strlen(path), tarName, my_strlen(tarName) + 1);
    int fd = flag ? open(fullPath, O_CREAT|O_RDWR|O_TRUNC, 0644) : 1;
    for(; *i < numArgs; *i = *i + 1) {
        if(tarAdd(fd, args[*i])) return 1;
    }
    close(fd);
    return 0;
}