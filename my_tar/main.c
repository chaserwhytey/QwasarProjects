#include "my_tar.h"
#include <stdio.h>
#include <stdlib.h>

int main(int ac, char** av) {
    int mode = 0;
    int fFlag = 0;
    char path[255] = "./";
    char* tarName = NULL;
    if(ac < 1) {
        printf("error: Need more arguments");
    }
    for(int i = 1; i < ac; i++) {
        if(av[i][0] == '-') {
            if(handleOptions(av, path, &i, &fFlag, &mode)) return 1;
        }
        else {
            if (tar(av, fFlag, tarName, &i, path, mode, ac)) return 1;
        }
    }
    return 0;
}
