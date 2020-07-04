#include "my_tar.h"
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <dirent.h>
#include <utime.h>

void utoa(unsigned int val, int base, int size, int fd){
	char* buf = (char*) malloc(32);
    buf[31] = 0;
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = nums[val % base];
	int len = write(fd, &buf[i+1], strlen(&buf[i + 1]));
    lseek(fd, size - len, SEEK_CUR);
    free(buf);
}

unsigned long oct2num(char* oct, int base, int size) {
    int i = size;
    while(oct[--i] == '\0') {}
    int ret = 0;
    int step = 1;
    for (; i>= 0; i--){
        ret+=(oct[i] - 48)*step;
        step*=base;
    }
    return ret;
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
    char fullPath[strlen(path) + sizeof(head->name) + 1];
    strncpy(fullPath, path, strlen(path));
    while(extractTarEntry(&head, fd)) {
        struct stat fileStat;
        unsigned long size = oct2num(head->size, 8, sizeof(head->size));
        unsigned long modTime = oct2num(head->modified, 8, sizeof(head->modified));
        unsigned int mode = oct2num(head->mode, 8, sizeof(head->mode));
        strncpy(fullPath + strlen(path), head->name, sizeof(head->name));
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

int readEntry(int fd) {
    tar_header* head = (tar_header*)malloc(sizeof(tar_header));
    int ret = read(fd, head->name, sizeof(head->name));
    if(ret != 0) {
        printf("%s\n", head->name);
        lseek(fd, sizeof(head->mode) + sizeof(head->owner) + sizeof(head->group), SEEK_CUR);
        read(fd, head->size, sizeof(head->size));
        int size = oct2num(head->size, 8, sizeof(head->size));
        lseek(fd, sizeof(head->modified), SEEK_CUR);
        read(fd, &head->type, sizeof(head->type));
        lseek(fd, sizeof(head->linkNum) + sizeof(head->link) + sizeof(head->padding), SEEK_CUR);
        unsigned int offset = size % BLOCK_SIZE;
        if(offset != 0) {
            offset = BLOCK_SIZE - offset;
        }
        size = head->type == DIRECTORY ? 0 : size + offset;
        lseek(fd, size, SEEK_CUR);
    }
    free(head);
    return ret;
}

void tarList(char* tarName) {
    int fd = open(tarName, O_RDONLY);
    while(readEntry(fd)) {      
    }
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

int tarAdd(int fd, char* arg) {
    struct stat fileStat;
    if(stat(arg, &fileStat) < 0) {   
        printf("path name %s does not exist", arg); 
        return 1;
    }
    
    tar_header* head = (tar_header*)malloc(sizeof(tar_header));
    strncpy(head->name, arg, strlen(arg) + 1);
    int len = write(fd, head->name, strlen(head->name));
    lseek(fd, sizeof(head->name) - len, SEEK_CUR);
    utoa(fileStat.st_mode & 0777, 8, sizeof(head->mode), fd);
    utoa(fileStat.st_uid, 8, sizeof(head->owner), fd);
    utoa(fileStat.st_gid, 8, sizeof(head->group), fd);
    utoa(fileStat.st_size, 8, sizeof(head->size), fd);
    utoa(fileStat.st_mtime, 8, sizeof(head->modified), fd);
    
    switch(fileStat.st_mode & S_IFMT) {
        case S_IFREG: head->type = REGULAR; break;
        case S_IFDIR: head->type = DIRECTORY; break;
        case S_IFCHR: head->type = CHAR; break;
        case S_IFBLK: head->type = BLOCK; break;
        case S_IFIFO: head->type = FIFO; break;
        case S_IFSOCK: printf("error: cannot tar socket"); return 1; break;
        default: printf("error: unknown file type"); return 1; break;
    }

    write(fd, &head->type, 1);
    utoa(fileStat.st_nlink, 10, sizeof(head->linkNum), fd);

    errno = 0;
    ssize_t bufSize = readlink(arg, head->link, 100);
    if(errno != 0 && errno != EINVAL) {
        printf("error: reading link");
        return 1;
    }
    if(bufSize == 100) {
        printf("error: sym link path too long");
    }
    if(bufSize < 100 && bufSize > 0) {
        head->type = SYMLINK;
        head->link[strlen(head->link)] = '\0';
        write(fd, head->link, bufSize);
        lseek(fd, sizeof(head->link) - bufSize, SEEK_CUR);
    }
    else {
        lseek(fd, sizeof(head->link), SEEK_CUR);
    }
    lseek(fd, sizeof(head->padding), SEEK_CUR);
    if(head->type == DIRECTORY) {
        struct dirent *de;  
        DIR *dp;
        dp = opendir(arg);
        while((de = readdir(dp))) {
            char buffer[200];
            strncpy(buffer, head->name, sizeof(head->name));
            buffer[strlen(buffer)] = '/';
            strncpy(buffer + strlen(buffer), de->d_name, strlen(de->d_name));
            if(de->d_name[0] != '.') 
                tarAdd(fd, buffer);
        }
        closedir(dp);
    }
    else {
        int readFile = open(arg, O_RDONLY);
        int fileSize = fileStat.st_size;
        char buffer[fileSize];
        read(readFile, buffer, fileSize);
        write(fd, buffer, fileSize);
        close(readFile);
        for(int i = 0; i < BLOCK_SIZE - (fileSize % BLOCK_SIZE == 0 ? BLOCK_SIZE : fileSize % BLOCK_SIZE); i++) {
            write(fd, "\0", 1);
        }
    }
    free(head);
    return 0;
}

int createTar(char* tarName, char** args, char* path, int flag, int* i, int numArgs) {
    char fullPath[200];
    strncpy(fullPath, path, strlen(path));
    strncpy(fullPath + strlen(path), tarName, strlen(tarName) + 1);
    int fd = flag ? open(fullPath, O_CREAT|O_RDWR|O_TRUNC, 0644) : 1;
    for(; *i < numArgs; *i = *i + 1) {
        if(tarAdd(fd, args[*i])) return 1;
    }
    close(fd);
    return 0;
}