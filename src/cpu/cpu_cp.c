#include "cpu.h"

void cpu_cp(cpu_t *cpu, uint8_t val) {
    uint8_t a = cpu->registers.a;
    int z = (a == val);
    int n = 1;
    int h = ((a & 0x0F) < (val & 0x0F));
    int c = (a < val);

    cpu->registers.f = (z << 7) | (n << 6) | (h << 5) | (c << 4);
}