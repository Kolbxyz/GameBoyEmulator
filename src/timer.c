#include "cpu.h"

void update_timers(cpu_t *cpu, int cycles)
{
    uint8_t tac = cpu->memory[0xFF07];
    if (tac & 0x04) {
        static const int bit_table[] = {9, 3, 5, 7};
        int bit = bit_table[tac & 0x03];
        uint16_t prev = cpu->div_counter;
        uint16_t next = prev + (uint16_t)cycles;

        /* Count falling edges of the selected bit in [prev+1, next] */
        int edges;
        if (next >= prev) {
            edges = (next >> (bit + 1)) - (prev >> (bit + 1));
        } else {
            /* uint16_t wraparound */
            edges = ((0x10000 >> (bit + 1)) - (prev >> (bit + 1)))
                  + (next >> (bit + 1));
        }
        for (int i = 0; i < edges; i++) {
            uint8_t tima = cpu->memory[0xFF05];
            if (tima == 0xFF) {
                cpu->memory[0xFF05] = cpu->memory[0xFF06];
                cpu->memory[0xFF0F] |= 0x04;
            } else {
                cpu->memory[0xFF05] = tima + 1;
            }
        }
    }

    cpu->div_counter += cycles;
    cpu->memory[0xFF04] = (cpu->div_counter >> 8);

    if (cpu->serial_timer > 0) {
        cpu->serial_timer -= cycles;
        if (cpu->serial_timer <= 0) {
            cpu->serial_timer = 0;
            cpu->memory[0xFF02] &= 0x7F;
            cpu->memory[0xFF01] = 0xFF;
            cpu->memory[0xFF0F] |= 0x08;
        }
    }
}