#include "cpu.h"

uint8_t cpu_srl(cpu_t *cpu, uint8_t val) {
    uint8_t carry = val & 1;
    uint8_t res = val >> 1;

    cpu->registers.f = (res == 0 ? FLAG_Z : 0) | (carry << 4);
    return res;
}

uint8_t cpu_rr(cpu_t *cpu, uint8_t val) {
    uint8_t old_c = (cpu->registers.f & FLAG_C) ? 1 : 0;
    uint8_t new_c = val & 1;
    uint8_t res = (val >> 1) | (old_c << 7);

    cpu->registers.f = (res == 0 ? FLAG_Z : 0) | (new_c << 4);
    return res;
}