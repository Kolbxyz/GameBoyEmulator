#include "cpu.h"

uint8_t read_8(cpu_t *cpu, uint16_t addr) {
    uint8_t byte = cpu->memory[addr];

    return byte;
}
