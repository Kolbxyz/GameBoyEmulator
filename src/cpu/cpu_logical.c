#include "cpu.h"

void cpu_and(cpu_t *cpu, uint8_t val) {
    int z = 0;
    cpu->registers.a &= val;
    z = (cpu->registers.a == 0);

    cpu->registers.f = (z << 7) | (1 << 5);
}

void cpu_xor(cpu_t *cpu, uint8_t val) {
    int z = 0;

    cpu->registers.a ^= val;
    z = (cpu->registers.a == 0);
    cpu->registers.f = (z << 7);
}

void cpu_or(cpu_t *cpu, uint8_t val) {
    int z = 0;

    cpu->registers.a |= val;
    z = (cpu->registers.a == 0);
    cpu->registers.f = (z << 7);
}