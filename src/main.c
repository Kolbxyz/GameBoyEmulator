#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"

void init_cpu(cpu_t *cpu)
{
    cpu->sp = 0xFFFE;
    cpu->pc = 0x0100;
    cpu->registers.a = 0x01;
}

void execute_cb_instruction(cpu_t *cpu, uint8_t opcode)
{
    switch (opcode) {
                case 0x30: { // SWAP B
            uint8_t val = cpu->registers.b;
            cpu->registers.b = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);
            cpu->registers.f = (cpu->registers.b == 0) ? 0x80 : 0x00;
            break;
        
            // CB OPCODES
    }
}

int main() {
    int counter = 0;
    cpu_t cpu = {0};
    uint8_t opcode = 0x0;

    read_rom("./assets/tetris.gb", &cpu);
    init_cpu(&cpu);
    while (++counter) {
        if (cpu.ime_scheduled == 1)
            cpu.ime = cpu.ime_scheduled--;
        opcode = cpu.memory[cpu.pc];
        // printf("> [%d](%#4x) %#4x\n", counter, cpu.pc, opcode); // Commenté pour alléger le log si besoin
        switch (opcode) {
                        case 0x00: INCR(1); // NOP

            // MAIN OPCODES
        }
    }
    return SUCCESS;
}
