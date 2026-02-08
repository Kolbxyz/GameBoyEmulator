#include "cpu.h"

uint16_t read_16(cpu_t *cpu, uint16_t addr) {
    uint8_t lo = cpu->memory[addr];
    uint8_t hi = cpu->memory[addr + 1];

    return (hi << 8) | lo;
}