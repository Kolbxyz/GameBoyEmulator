#include "cpu.h"

/*
** Game Boy Timers Implementation
** DIV (0xFF04): Increments at 16384 Hz (every 256 cycles)
** TIMA (0xFF05): Main timer, increments based on TAC frequency
** TMA (0xFF06): Value loaded into TIMA when it overflows
** TAC (0xFF07): Timer Control (Bit 2: Enable, Bits 0-1: Frequency)
*/
void update_timers(cpu_t *cpu, int cycles)
{
    static uint16_t div_counter = 0;
    uint16_t prev_div = div_counter;
    div_counter += cycles;

    cpu->memory[0xFF04] = (div_counter >> 8);

    uint8_t tac = cpu->memory[0xFF07];
    if (tac & 0x04) {
        int bit = 9; // 4096 Hz
        switch (tac & 0x03) {
            case 0x01: bit = 3; break; // 262144 Hz
            case 0x02: bit = 5; break; // 65536 Hz
            case 0x03: bit = 7; break; // 16384 Hz
        }

        // Increment TIMA on falling edge of selected bit
        if (((prev_div >> bit) & 0x01) && !((div_counter >> bit) & 0x01)) {
            if (read_8(cpu, 0xFF05) == 0xFF) {
                write_8(cpu, 0xFF05, read_8(cpu, 0xFF06));
                write_8(cpu, 0xFF0F, read_8(cpu, 0xFF0F) | 0x04);
                // printf("[TIMER] Overflow! Interrupt triggered.\n");
            } else {
                write_8(cpu, 0xFF05, read_8(cpu, 0xFF05) + 1);
            }
        }
    }
}