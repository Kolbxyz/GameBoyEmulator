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
            // MAIN OPCODES
        }
    }
    return SUCCESS;
}
