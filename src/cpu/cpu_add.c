#include "cpu.h"
#include <stdint.h>

void cpu_add(cpu_t *cpu, uint8_t value)
{
    uint8_t res = cpu->registers.a + value;
    int n, z, h, c = 0;

    z = ((res & 0xFF) == 0);
    h = ((cpu->registers.a & 0xF) + (value & 0xF)) > 0xF;
    c = (res > 0xFF);
    cpu->registers.f = (z << 7) | (n << 6) | (h << 5) | (c << 4);
    cpu->registers.a = (uint8_t) res;
}

void cpu_adc(cpu_t *cpu, uint8_t val) {
    int carry = (cpu->registers.f & FLAG_C) ? 1 : 0;
    uint16_t result = cpu->registers.a + val + carry;
    int z = ((result & 0xFF) == 0);
    int n = 0;
    int h = ((cpu->registers.a & 0x0F) + (val & 0x0F) + carry) > 0x0F;
    int c = result > 0xFF;

    cpu->registers.a = (uint8_t)result;
    cpu->registers.f = (z << 7) | (n << 6) | (h << 5) | (c << 4);
}

void cpu_add_hl(cpu_t *cpu, uint16_t val) {
    uint32_t result = cpu->registers.hl + val;
    int z = (cpu->registers.f & 0x80) ? 1 : 0;
    int n = 0;
    int h = ((cpu->registers.hl & 0xFFF) + (val & 0xFFF)) > 0xFFF;
    int c = result > 0xFFFF;

    cpu->registers.hl = (uint16_t)result;
    cpu->registers.f = (z << 7) | (n << 6) | (h << 5) | (c << 4);
}
