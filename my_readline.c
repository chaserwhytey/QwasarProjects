#define READLINE_READ_SIZE 512
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char* doubleSize(int stringInd, int* size, char* s) {
    if(stringInd == *size) {
        char* new = (char*)malloc(*size*2);
        for(int i = 0; i < *size; i++) {
            new[i] = s[i];
        }
        *size *= 2;
        free(s);
        s = new;
    }
    return s;
}

void readline(char* buffer, int* stringInd, int* size, char** s, int* newline) {
    int index = 0;
    while(buffer[index] == '\0') {index++;}
    while(buffer[index] != '\0' && index < READLINE_READ_SIZE) {
        if(buffer[index] == '\n') {
            *newline = 1;
            buffer[index++] = '\0';
            break;
        }
        s[0][*stringInd] = buffer[index];
        *stringInd += 1;
        *s = doubleSize(*stringInd, size, *s);
        buffer[index++] = '\0';
    }
    if(index == READLINE_READ_SIZE || buffer[index] == '\0') {
        buffer[READLINE_READ_SIZE] = '\0';
    }
}

char* my_readline(int fd) {
    static char buffer[READLINE_READ_SIZE + 1] = { 0 };
    int bytesRead;
    int newline = 0;
    int size = READLINE_READ_SIZE;
    char *s = (char*)malloc(size); 
    int stringInd = 0;
    while(!newline){
        if(buffer[READLINE_READ_SIZE] == '\0') {
            if(!(bytesRead = read(fd, buffer, READLINE_READ_SIZE))) return NULL;
            if(bytesRead < READLINE_READ_SIZE) {
                buffer[bytesRead] = '\0';
            }
            buffer[READLINE_READ_SIZE] = '1';
        }
        readline(buffer, &stringInd, &size, &s, &newline);
    }
    s[stringInd] = '\0';
    return s;
}

int main(int ac, char **av)
{
  char *str = NULL;

  int fd = open("./test", O_RDONLY);
  while ((str = my_readline(fd)) != NULL)
  {
      printf("%s\n", str);
      free(str);
  }
  close(fd);
  //
  //  Yes it's also working with stdin :-)
  //printf("%s", my_readline(0));
  //

  return 0;
}
