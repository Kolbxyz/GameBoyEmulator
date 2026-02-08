#include "cpu.h"
#include <stdlib.h>
#include <stdio.h>

void read_rom(const char *path, cpu_t *cpu)
{
    size_t read_bytes = 0;
    FILE *f = fopen(path, "rb");

    if (f == NULL)
        THROW("Couldn't read that file.", INVALID_FILE);
    read_bytes = fread(cpu->memory, sizeof(uint8_t), 32768, f);
    fclose(f);
}