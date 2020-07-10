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
        if((*av[i] == '-') && *(av[i] + 1) == 'C') {
            if( !(av[i+1]) ) {
                printf("error: directory not specified.\n");
                return 1;
            }
            my_strncpy(path, av[i + 1], my_strlen(av[i + 1]) + 1);
            path[my_strlen(path)] = '/';
            printf("%s\n", path);
            i++;
        }

        else if(*av[i] == '-') {
            if(setFlags(&mode, &fFlag, av[i] + 1))
                return 1;
        }
        else {
            char buffer[100] = {0};
            if(fFlag) {
                my_strncpy(&buffer[my_strlen(buffer)], av[i], my_strlen(av[i]) + 1);
                i++;
                tarName = buffer;
            }
            switch(mode) {
                case cMode: 
                    createTar(tarName, av, path, fFlag, &i, ac);
                    break;
                case tMode:
                    if (i < ac - 1) {
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
                    if(tarAppend(tarName, av, &i, ac)) return 1;
                    break;
                case uMode:
                    if(!fFlag) {
                        printf("error: archive -f must be specified\n");
                        return 1;
                    }
                    if(updateEntry(tarName, av, i, ac)) return 1;
                    i = ac;
                    break;
                default: 
                    printf("error: no mode specified\n");
                    return 1;
                    break;
            }
        }
    }
    return 0;
}