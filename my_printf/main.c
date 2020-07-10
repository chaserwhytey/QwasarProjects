#include "my_printf.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    int p = 0;
    void* t = malloc(10);
    my_printf("NULL STRING %s! hexNum %p octNum %d\n", (char*)NULL, t, p);
    free(t);
}