#include "my_ls.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

int main(int ac, char** av) {
    wordnode* filelist = NULL;
    wordnode* dirlist = NULL;
    int sFlag = 0;
    int tFlag = 0;
    int vars = 1;
    for(int i = 1; i < ac; i++) {
        char* temp = av[i];
        if(temp[0] == '-') {
            setFlags(&sFlag, &tFlag, av[1] + 1);
        }
        else {
            vars = 0;
            struct stat statbuf;
            time_t time = statbuf.st_mtime + (double)statbuf.st_mtim.tv_nsec/1000000000;
            if (stat(av[i], &statbuf) != 0) {
                printf("There was an error, stat failed");
                return 0;
            }
            if(isDirectory(temp)){
                dirlist = insertWord(dirlist, temp, tFlag, time);
            }
            else if(isFile(temp)) {
                filelist = insertWord(filelist, temp, tFlag, time);
            }
            else printf("%s does not exist", temp);
        }
    }
    if(vars) {
        readContents(".", sFlag, tFlag);
    }
    //print list
    printList(filelist);
    printDirList(dirlist, sFlag, tFlag);

    // clean up memory
    freemem(filelist);
    freemem(dirlist);
}