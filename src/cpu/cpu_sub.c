#include "cpu.h"

void cpu_sbc(cpu_t *cpu, uint8_t val) {
    int carry = (cpu->registers.f & FLAG_C) ? 1 : 0;
    int result = cpu->registers.a - val - carry;
    int z = ((result & 0xFF) == 0);
    int n = 1;
    int h = ((cpu->registers.a & 0x0F) - (val & 0x0F) - carry) < 0;
    int c = result < 0;

    cpu->registers.a = (uint8_t)result;
    cpu->registers.f = (z << 7) | (n << 6) | (h << 5) | (c << 4);
}

void cpu_sub(cpu_t *cpu, uint8_t val) {
    uint8_t a = cpu->registers.a;
    int result = a - val;

    int z = ((result & 0xFF) == 0);
    int n = 1;
    int h = ((a & 0x0F) < (val & 0x0F));
    int c = (a < val);

    cpu->registers.a = (uint8_t)result;
    cpu->registers.f = (z << 7) | (n << 6) | (h << 5) | (c << 4);
}
