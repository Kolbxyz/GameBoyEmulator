#include "cpu.h"

uint8_t cpu_rlc(cpu_t *cpu, uint8_t val) {
    uint8_t carry = (val >> 7) & 1;
    uint8_t res = (val << 1) | carry;

    cpu->registers.f = (res == 0 ? FLAG_Z : 0) | (carry << 4);
    return res;
}

uint8_t cpu_rrc(cpu_t *cpu, uint8_t val) {
    uint8_t carry = val & 1;
    uint8_t res = (val >> 1) | (carry << 7);

    cpu->registers.f = (res == 0 ? FLAG_Z : 0) | (carry << 4);
    return res;
}

uint8_t cpu_rl(cpu_t *cpu, uint8_t val) {
    uint8_t old_c = (cpu->registers.f & FLAG_C) ? 1 : 0;
    uint8_t carry = (val >> 7) & 1;
    uint8_t res = (val << 1) | old_c;

    cpu->registers.f = (res == 0 ? FLAG_Z : 0) | (carry << 4);
    return res;
}

uint8_t cpu_sla(cpu_t *cpu, uint8_t val) {
    uint8_t carry = (val >> 7) & 1;
    uint8_t res = val << 1;

    cpu->registers.f = (res == 0 ? FLAG_Z : 0) | (carry << 4);
    return res;
}

uint8_t cpu_sra(cpu_t *cpu, uint8_t val) {
    uint8_t carry = val & 1;
    uint8_t res = (val >> 1) | (val & 0x80);

    cpu->registers.f = (res == 0 ? FLAG_Z : 0) | (carry << 4);
    return res;
}

uint8_t cpu_swap(cpu_t *cpu, uint8_t val) {
    uint8_t res = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);

    cpu->registers.f = (res == 0 ? FLAG_Z : 0);
    return res;
}

void cpu_bit(cpu_t *cpu, uint8_t bit, uint8_t val) {
    int z = !(val & (1 << bit));

    cpu->registers.f = (z << 7) | (0 << 6) | (1 << 5) | (cpu->registers.f & FLAG_C);
}

uint8_t cpu_res(uint8_t bit, uint8_t val) {
    return val & ~(1 << bit);
}

uint8_t cpu_set(uint8_t bit, uint8_t val) {
    return val | (1 << bit);
}
