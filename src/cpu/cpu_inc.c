#include "cpu.h"

void cpu_inc(cpu_t *cpu, uint8_t *reg) {
    uint8_t result = ++(*reg);
    int z = (result == 0);
    int n = 0;
    int h = ((result & 0x0F) == 0x00);
    int current_c = (cpu->registers.f & FLAG_C) ? 1 : 0;

    cpu->registers.f = (z << 7) | (n << 6) | (h << 5) | (current_c << 4);
}