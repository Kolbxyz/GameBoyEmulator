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

                        OP_LD_16(0x01, bc) // LD BC, u16

                        case 0x02: { // LD (BC), A
                cpu.memory[cpu.registers.bc] = cpu.registers.a;
                INCR(1);
            }

                        case 0x03: { // INC BC
                cpu.registers.bc++;
                INCR(1);
            }

            // --- INC r (Increment 8-bit) ---
                        case 0x04: cpu_inc(&cpu, &cpu.registers.b); INCR(1); // INC B

                        case 0x05: { // DEC B
                cpu_dec(&cpu, &cpu.registers.b);
                INCR(1);
            }

                        OP_LD_8(0x06, b) // LD B, u8

                        case 0x07: { // RLCA (Rotate Left Circular Accumulator)
                uint8_t a = cpu.registers.a;
                uint8_t carry = (a >> 7);
                cpu.registers.a = (a << 1) | carry;
                cpu.registers.f = (0 << 7) | (0 << 6) | (0 << 5) | (carry << 4);
                INCR(1);
            }

                        case 0x09: { // ADD HL, BC
                cpu_add_hl(&cpu, cpu.registers.bc);
                INCR(1);
            }

                        case 0x0A: { // LD A, (BC)
                cpu.registers.a = cpu.memory[cpu.registers.bc];
                INCR(1);
            }

                        case 0x0B: { // DEC BC
                cpu.registers.bc--;
                INCR(1);
            }

                        case 0x0C: cpu_inc(&cpu, &cpu.registers.c); INCR(1); // INC C
                        case 0x0D: { // DEC C
                cpu_dec(&cpu, &cpu.registers.c);
                INCR(1);
            }

                        OP_LD_8(0x0E, c) // LD C, u8

                        case 0x0F: { // RRCA (Rotate Right Circular Accumulator)
                uint8_t a = cpu.registers.a;
                uint8_t carry = (a & 1);
                cpu.registers.a = (a >> 1) | (carry << 7);
                cpu.registers.f = (0 << 7) | (0 << 6) | (0 << 5) | (carry << 4);
                INCR(1);
            }

                        case 0x10: { // STOP
                INCR(2);
            }

                        OP_LD_16(0x11, de) // LD DE, u16

                        case 0x12: { // LD (DE), A
                cpu.memory[cpu.registers.de] = cpu.registers.a;
                INCR(1);
            }
                        case 0x13: { // INC DE
                cpu.registers.de++;
                INCR(1);
            }
                        case 0x14: cpu_inc(&cpu, &cpu.registers.d); INCR(1); // INC D

                        case 0x15: cpu_dec(&cpu, &cpu.registers.d); INCR(1); // DEC D

                        case 0x16: { // LD D, u8
                cpu.registers.d = cpu.memory[cpu.pc + 1];
                INCR(2);
            }

                        case 0x17: { // RLA (Rotate Left Accumulator through Carry) - Manquait !
                uint8_t a = cpu.registers.a;
                uint8_t old_c = (cpu.registers.f & FLAG_C) ? 1 : 0;
                uint8_t new_c = (a >> 7);
                cpu.registers.a = (a << 1) | old_c;
                cpu.registers.f = (0 << 7) | (0 << 6) | (0 << 5) | (new_c << 4);
                INCR(1);
            }

                        case 0x19: { // ADD HL, DE
                cpu_add_hl(&cpu, cpu.registers.de);
                INCR(1);
            }

                        case 0x1A: { // LD A, (DE)
                cpu.registers.a = cpu.memory[cpu.registers.de];
                INCR(1);
            }

                        case 0x1B: cpu.registers.de--; INCR(1); // DEC DE - Manquait !
                        case 0x1C: cpu_inc(&cpu, &cpu.registers.e); INCR(1); // INC E
                        case 0x1D: cpu_dec(&cpu, &cpu.registers.e); INCR(1); // DEC E

                        case 0x1E: { // LD E, u8
                cpu.registers.e = cpu.memory[cpu.pc + 1];
                INCR(2);
            }

                        case 0x1F: { // RRA (Rotate Right Accumulator through Carry) - Manquait !
                uint8_t a = cpu.registers.a;
                uint8_t old_c = (cpu.registers.f & FLAG_C) ? 1 : 0;
                uint8_t new_c = (a & 1);
                cpu.registers.a = (a >> 1) | (old_c << 7);
                cpu.registers.f = (0 << 7) | (0 << 6) | (0 << 5) | (new_c << 4);
                INCR(1);
            }

                        case 0x20: { // JR NZ, r8
                int8_t offset = (int8_t)cpu.memory[cpu.pc + 1];
                cpu.pc += 2;
                if (!(cpu.registers.f & 0x80)) {
                    cpu.pc += offset;
                }
                break;
            }

                        OP_LD_16(0x21, hl) // LD HL, u16

                        case 0x22: { // LD (HL+), A
                cpu.memory[cpu.registers.hl++] = cpu.registers.a;
                INCR(1);
            }

                        case 0x23: { // INC HL
                cpu.registers.hl++;
                INCR(1);
            }

                        case 0x24: cpu_inc(&cpu, &cpu.registers.h); INCR(1); // INC H

                        case 0x25: cpu_dec(&cpu, &cpu.registers.h); INCR(1); // DEC H

                        case 0x26: { // LD H, u8
                cpu.registers.h = cpu.memory[cpu.pc + 1];
                INCR(2);
            }

                        case 0x27: { // DAA
                int a = cpu.registers.a;
                if ((cpu.registers.f & FLAG_N) == 0) {
                    if ((cpu.registers.f & FLAG_H) || (a & 0x0F) > 9) a += 0x06;
                    if ((cpu.registers.f & FLAG_C) || a > 0x9F) { a += 0x60; cpu.registers.f |= FLAG_C; }
                } else {
                    if (cpu.registers.f & FLAG_H) a -= 0x06;
                    if (cpu.registers.f & FLAG_C) a -= 0x60;
                }
                cpu.registers.a = (uint8_t)a;
                if (cpu.registers.a == 0) cpu.registers.f |= FLAG_Z;
                else cpu.registers.f &= ~FLAG_Z;
                cpu.registers.f &= ~FLAG_H;
                INCR(1);
            }

                        case 0x29: { // ADD HL, HL
                cpu_add_hl(&cpu, cpu.registers.hl);
                INCR(1);
            }

                        case 0x2A: { // LD A, (HL+)
                cpu.registers.a = cpu.memory[cpu.registers.hl++];
                INCR(1);
            }

                        case 0x2B: cpu.registers.hl--; INCR(1); // DEC HL - Manquait !

                        case 0x2C: cpu_inc(&cpu, &cpu.registers.l); INCR(1); // INC L

                        case 0x2D: cpu_dec(&cpu, &cpu.registers.l); INCR(1); // DEC L

                        case 0x2E: { // LD L, u8
                cpu.registers.l = cpu.memory[cpu.pc + 1];
                INCR(2);
            }

                        case 0x2F: { // CPL
                cpu.registers.a = ~cpu.registers.a;
                cpu.registers.f |= 0x60;
                INCR(1);
            }

                        case 0x31: { // LD SP, u16
                cpu.sp = read_16(&cpu, cpu.pc + 1);
                INCR(3);
            }

                        case 0x32: { // LD (HL-), A
                cpu.memory[cpu.registers.hl--] = cpu.registers.a;
                INCR(1);
            }

                        case 0x33: { // INC SP
                cpu.sp++;
                INCR(1);
            }

                        case 0x34: { // INC (HL)
                uint8_t val = cpu.memory[cpu.registers.hl] + 1;
                int z = (val == 0);
                int h = ((val & 0x0F) == 0x00);
                int current_c = (cpu.registers.f & FLAG_C) ? 1 : 0;
                cpu.memory[cpu.registers.hl] = val;
                cpu.registers.f = (z << 7) | (0 << 6) | (h << 5) | (current_c << 4);
                INCR(1);
            }

                        case 0x35: { // DEC (HL)
                uint8_t val = cpu.memory[cpu.registers.hl];
                cpu_dec(&cpu, &val);
                cpu.memory[cpu.registers.hl] = val; 
                INCR(1);
            }

                        OP_LD_8(0x36, hl) // LD (HL), u8

                        case 0x39: { // ADD HL, SP
                cpu_add_hl(&cpu, cpu.sp);
                INCR(1);
            }

                        case 0x3A: { // LD A, (HL-)
                cpu.registers.a = cpu.memory[cpu.registers.hl--];
                INCR(1);
            }

                        case 0x3B: cpu.sp--; INCR(1); // DEC SP

                        case 0x3D: cpu_dec(&cpu, &cpu.registers.a); INCR(1); // DEC A

                        OP_LD_8(0x3E, b) // LD A, u8

            // --- LD B, r ---
                        case 0x40: cpu.registers.b = cpu.registers.b; INCR(1); // LD B, B
                        case 0x41: cpu.registers.b = cpu.registers.c; INCR(1); // LD B, C
                        case 0x42: cpu.registers.b = cpu.registers.d; INCR(1); // LD B, D
                        case 0x43: cpu.registers.b = cpu.registers.e; INCR(1); // LD B, E
                        case 0x44: cpu.registers.b = cpu.registers.h; INCR(1); // LD B, H
                        case 0x45: cpu.registers.b = cpu.registers.l; INCR(1); // LD B, L
                        case 0x46: cpu.registers.b = cpu.memory[cpu.registers.hl]; INCR(1); // LD B, (HL)
                        case 0x47: cpu.registers.b = cpu.registers.a; INCR(1); // LD B, A

            // --- LD C, r ---
            // MAIN OPCODES
        }
    }
    return SUCCESS;
}
