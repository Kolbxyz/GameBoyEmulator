#include "cpu.h"

void stack_push16(cpu_t *cpu, uint16_t value) {
    cpu->memory[--cpu->sp] = (value >> 8) & 0xFF;
    cpu->memory[--cpu->sp] = (value & 0xFF);
}

uint16_t stack_pop16(cpu_t *cpu) {
    uint8_t hi = 0;
    uint8_t lo = cpu->memory[cpu->sp++];

    hi = cpu->memory[cpu->sp++];
    return (hi << 8) | lo;
}