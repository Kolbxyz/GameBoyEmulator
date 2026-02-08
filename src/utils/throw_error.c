#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

void throw_error(char *msg, error_t code, char *FILE, int LINE)
{
    fprintf(stderr, "[ERROR]: %s -> %s:%d", msg, FILE, LINE);
    exit(code);
}