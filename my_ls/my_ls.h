#ifndef MY_LS_H 
#define MY_LS_H 
#include <time.h>

typedef struct wordnode {
  char* word;
  struct wordnode* next;
  int timeMod;
} wordnode;

void freemem(wordnode* wordlist);
int isDirectory(const char *path);
wordnode* insertFront(wordnode* wordlist, char* nextWord, time_t time);
wordnode* insertWordAlpha(wordnode* wordlist, char* nextWord, int tFlag, time_t insertTime);
wordnode* insertWordTime(wordnode* wordlist, char* nextWord, time_t time);
wordnode* insertWord(wordnode* wordlist, char* nextWord, int tFlag, time_t timeMod);
void printList(wordnode* wordlist);
void freemem(wordnode* wordlist);
int isFile(const char *path);
void setFlags(int* sFlag, int* tFlag, char* flags);
time_t getTime(char* dir, char* d_name);
void readContents(char* dir, int sFlag, int tFlag);
void printDirList (wordnode* dirlist, int sFlag, int tFlag);

#endif