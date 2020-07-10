#include <stdarg.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void my_putchar(char c) {
    write(1, &c, 1);
}
void my_putstr(char* temp) {
    if(!temp) {
        return;
    }
    while(*temp) {
        my_putchar(*(temp++));
    }
}

int utoa(long long val, int base){
    if(val == 0) {
        my_putstr("0");
        return 1;
    }
    int ret = 0;
    int isNegative = 0;
    if(val < 0) {
        isNegative = 1;
        val*=-1;
    }
	char* buf = (char*) malloc(32);
    buf[31] = 0;
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = hexNums[val % base];
    if(isNegative) {
        my_putstr("-");
        ret++;
    }
	my_putstr(&buf[i+1]);
    ret += strlen(&buf[i+1]);
    free(buf);
    return ret;
}

int my_printf(char* format, ...) {
    int length = 0;
    va_list ap;
    int d;
    unsigned int u;
    unsigned long a;
    char c;
    char* s;
    va_start(ap, format);
    while(*format) {
        if(*format == '%') {
            switch (*(++format)) {
                case 's':              /* string */
                    s = va_arg(ap, char *);
                    if(s) {
                        length += strlen(s);
                        my_putstr(s);
                    }
                    else {
                        length += 6;
                        my_putstr("(null)");
                    }
                    break;
                case 'd':              /* int */
                    d = va_arg(ap, int);
                    length += utoa(d, 10);
                    break;
                case 'o':              /* int */
                    u = va_arg(ap, unsigned int);
                    length += utoa(u, 8);
                    break;
                case 'u':              /* int */
                    u = va_arg(ap, unsigned int);
                    length += utoa(u, 10);
                    break;
                case 'x':              /* int */
                    u = va_arg(ap, unsigned int);
                    length += utoa(u, 16);
                    break;
                case 'p':              /* int */
                    a = (unsigned long) va_arg(ap, void*);
                    my_putstr("0x");
                    length += 2;
                    length += utoa(a, 16);
                    break;
                case 'c':              /* char */
                    /* need a cast here since va_arg only
                        takes fully promoted types */
                    c = (char) va_arg(ap, int);
                    my_putchar(c);
                    length++;
                    break;
            }
            
            format++;

        }
        else {
            my_putchar(*format++);
            length++;
        }
    }
    va_end(ap);
    return length;
}