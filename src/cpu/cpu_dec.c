#include "cpu.h"

void cpu_dec(cpu_t *cpu, uint8_t *reg) {
    uint8_t result = --(*reg);
    int z = (result == 0);
    int n = 1;
    int h = ((result & 0x0F) == 0x0F);
    int current_c = (cpu->registers.f & FLAG_C) ? 1 : 0;

    cpu->registers.f = (z << 7) | (n << 6) | (h << 5) | (current_c << 4);
}