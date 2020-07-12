#include "my_tar.h"
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>

int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

int my_strncmp(char* s1, char* s2, int length) {
    for(int i = 0; i < length; i++) {
        if(s1[i] != s2[i]) return 1;
    }
    return 0;
}

int my_strlen(char* c1) {
    int length = 0;
    while(*(c1++) != '\0') {
        length++;
    }
    return length;
}

void my_strncpy(char* c1, char* c2, int length) {
    for(int i = 0; i < length; i++) {
        c1[i] = c2[i];
    }
}

int getFileCount(char* path) {
    int file_count = 0;
    DIR * dirp;
    struct dirent * entry;
    dirp = opendir(path); 
    if(!dirp) {
        printf("directory %s does not exist", path);
        return -1;
    }
    while ((entry = readdir(dirp))) {
        if (entry->d_name[0] != '.') { 
            file_count++;
        }
    }
    closedir(dirp);
    return file_count;
}

void utoa(unsigned int val, int base, char* var){
	char buf[32];
    buf[31] = 0;
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = nums[val % base];
    my_strncpy(var, &buf[i+1], my_strlen(&buf[i+1]) + 1);
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
