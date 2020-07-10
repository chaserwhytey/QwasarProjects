#ifndef MY_PRINTF_H
#define MY_PRINTF_H

#define hexNums "0123456789abcdef"
void my_putchar(char c);
void my_putstr(char* temp);
int itoa(int val);
int utoa(unsigned long val, int base);
int utoa(unsigned long val, int base);
int my_printf(char* format, ...);

#endif