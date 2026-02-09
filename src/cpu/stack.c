#include "cpu.h"

void stack_push16(cpu_t *cpu, uint16_t value) {
    write_8(cpu, --cpu->sp, (value >> 8) & 0xFF);
    write_8(cpu, --cpu->sp, (value & 0xFF));
}

uint16_t stack_pop16(cpu_t *cpu) {
    uint8_t lo = read_8(cpu, cpu->sp++);
    uint8_t hi = read_8(cpu, cpu->sp++);
    return (hi << 8) | lo;
}