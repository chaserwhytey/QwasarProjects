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

void utoa(unsigned int val, int base, int size, int fd){
	char* buf = (char*) malloc(32);
    buf[31] = 0;
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = nums[val % base];
	int len = write(fd, &buf[i+1], strlen(&buf[i + 1]));
    for(int i = 0; i < size - len; i++) {
        write(fd, "\0", 1);
    }
    free(buf);
}

unsigned int oct2num(char* oct, int base, int size) {
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
    read(fd, (*head)->size, sizeof((*head)->size));
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
        unsigned int size = oct2num(head->size, 8, sizeof(head->size));
        unsigned int mode = oct2num(head->mode, 8, sizeof(head->mode));
        strncpy(fullPath + strlen(fullPath), head->name, sizeof(head->name));
        open(fullPath, O_CREAT|O_WRONLY, mode);
        lseek(fd, BLOCK_SIZE - sizeof(head->name) - sizeof(head->mode) - sizeof(head->owner) - sizeof(head->group) - sizeof(head->size), SEEK_CUR);
        int offset = size % BLOCK_SIZE;
        if(offset != 0) {
            offset = BLOCK_SIZE - offset;
        }
        size = size + offset;
        lseek(fd, size, SEEK_CUR);
        free(head);
        head = (tar_header*)malloc(sizeof(tar_header));
    }
    free(head);
    return 0;
}

void tarList(char* tarName) {
    int fd = open(tarName, O_RDONLY);
    tar_header* head = (tar_header*)malloc(sizeof(tar_header));
    
    while(read(fd, head->name, sizeof(head->name))) {
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
}
int tarAdd(int fd, char* arg, char* path, int* pos) {
    char fullPath[200] = { 0 };
    char relativeName[100] = { 0 };
    if(arg[0] != '/') {
        strncpy(fullPath, path, strlen(path));
    }
    strncpy(fullPath + strlen(fullPath), arg, strlen(arg));
    if(strlen(fullPath) == 200) {
        printf("Path name too long");
        return 1;
    }
    int idx = strlen(fullPath);
    while(fullPath[(--idx) - 1] != '/') {}
    strncpy(relativeName, fullPath + idx, strlen(fullPath + idx));
    struct stat fileStat;
    if(stat(fullPath, &fileStat) < 0) {   
        printf("path name %s does not exist", fullPath); 
        return 1;
    }
    tar_header* head = (tar_header*)malloc(sizeof(tar_header));
    int offset = *pos % BLOCK_SIZE;
    if(offset != 0) {
        offset = (BLOCK_SIZE - offset);
    }
    int offt = lseek(fd, offset, SEEK_CUR);
    printf("%d\n", offt);
    *pos += offset;
    strncpy(head->name, arg, strlen(arg) + 1);
    int len = write(fd, head->name, strlen(head->name));
    for(int i = 0; i < (int) (sizeof(head->name) - len); i++) {
        write(fd, "\0", 1);
    }
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
    ssize_t bufSize = readlink(fullPath, head->link, 100);
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
        for(int i = 0; i < (int) (sizeof(head->link) - bufSize); i++) {
            write(fd, "\0", 1);
        }
    }
    else {
        for(int i = 0; i < (int) sizeof(head->link); i++) {
            write(fd, "\0", 1);
        }
    }
    for(int i = 0; i < (int) sizeof(head->padding); i++) {
        write(fd, "\0", 1);
    }
    
    *pos += BLOCK_SIZE;
    if(head->type == DIRECTORY) {
        struct dirent *de;  
        DIR *dp;
        dp = opendir(fullPath);
        while((de = readdir(dp))) {
            char buffer[200];
            strncpy(buffer, head->name, sizeof(head->name));
            buffer[strlen(buffer)] = '/';
            strncpy(buffer + strlen(buffer), de->d_name, strlen(de->d_name));
            if(de->d_name[0] != '.') 
                tarAdd(fd, buffer, path, pos);
        }
        closedir(dp);
    }
    else {
        int readFile = open(fullPath, O_RDONLY);
        int fileSize = fileStat.st_size;
        char buffer[fileSize];
        read(readFile, buffer, fileSize);
        *pos += write(fd, buffer, fileSize);
        close(readFile);
    }
    free(head);
    return 0;
}

int createTar(char* tarName, char** args, char* path, int flag, int* i, int numArgs) {
    int fd = flag ? open(tarName, O_CREAT|O_RDWR|O_TRUNC, 0644) : 1;
    int pos = 0;
    for(; *i < numArgs; *i = *i + 1) {
        if(tarAdd(fd, args[*i], path, &pos)) return 1;
    }
    close(fd);
    return 0;
}