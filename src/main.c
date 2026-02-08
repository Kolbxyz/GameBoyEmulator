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
                        case 0x48: cpu.registers.c = cpu.registers.b; INCR(1); // LD C, B
                        case 0x49: cpu.registers.c = cpu.registers.c; INCR(1); // LD C, C
                        case 0x4A: cpu.registers.c = cpu.registers.d; INCR(1); // LD C, D
                        case 0x4B: cpu.registers.c = cpu.registers.e; INCR(1); // LD C, E
                        case 0x4C: cpu.registers.c = cpu.registers.h; INCR(1); // LD C, H
                        case 0x4D: cpu.registers.c = cpu.registers.l; INCR(1); // LD C, L
                        case 0x4E: cpu.registers.c = cpu.memory[cpu.registers.hl]; INCR(1); // LD C, (HL)
                        case 0x4F: cpu.registers.c = cpu.registers.a; INCR(1); // LD C, A

            // --- LD D, r ---
                        case 0x50: cpu.registers.d = cpu.registers.b; INCR(1); // LD D, B
                        case 0x51: cpu.registers.d = cpu.registers.c; INCR(1); // LD D, C
                        case 0x52: cpu.registers.d = cpu.registers.d; INCR(1); // LD D, D
                        case 0x53: cpu.registers.d = cpu.registers.e; INCR(1); // LD D, E
                        case 0x54: cpu.registers.d = cpu.registers.h; INCR(1); // LD D, H
                        case 0x55: cpu.registers.d = cpu.registers.l; INCR(1); // LD D, L
                        case 0x56: cpu.registers.d = cpu.memory[cpu.registers.hl]; INCR(1); // LD D, (HL)
                        case 0x57: cpu.registers.d = cpu.registers.a; INCR(1); // LD D, A

            // --- LD E, r ---
                        case 0x58: cpu.registers.e = cpu.registers.b; INCR(1); // LD E, B
                        case 0x59: cpu.registers.e = cpu.registers.c; INCR(1); // LD E, C
                        case 0x5A: cpu.registers.e = cpu.registers.d; INCR(1); // LD E, D
                        case 0x5B: cpu.registers.e = cpu.registers.e; INCR(1); // LD E, E
                        case 0x5C: cpu.registers.e = cpu.registers.h; INCR(1); // LD E, H
                        case 0x5D: cpu.registers.e = cpu.registers.l; INCR(1); // LD E, L
                        case 0x5E: cpu.registers.e = cpu.memory[cpu.registers.hl]; INCR(1); // LD E, (HL)
                        case 0x5F: cpu.registers.e = cpu.registers.a; INCR(1); // LD E, A

            // --- LD H, r ---
                        case 0x60: cpu.registers.h = cpu.registers.b; INCR(1); // LD H, B
                        case 0x61: cpu.registers.h = cpu.registers.c; INCR(1); // LD H, C
                        case 0x62: cpu.registers.h = cpu.registers.d; INCR(1); // LD H, D
                        case 0x63: cpu.registers.h = cpu.registers.e; INCR(1); // LD H, E
                        case 0x64: cpu.registers.h = cpu.registers.h; INCR(1); // LD H, H
                        case 0x65: cpu.registers.h = cpu.registers.l; INCR(1); // LD H, L
                        case 0x66: cpu.registers.h = cpu.memory[cpu.registers.hl]; INCR(1); // LD H, (HL)
                        case 0x67: cpu.registers.h = cpu.registers.a; INCR(1); // LD H, A

            // --- LD L, r ---
                        case 0x68: cpu.registers.l = cpu.registers.b; INCR(1); // LD L, B
                        case 0x69: cpu.registers.l = cpu.registers.c; INCR(1); // LD L, C
                        case 0x6A: cpu.registers.l = cpu.registers.d; INCR(1); // LD L, D
                        case 0x6B: cpu.registers.l = cpu.registers.e; INCR(1); // LD L, E
                        case 0x6C: cpu.registers.l = cpu.registers.h; INCR(1); // LD L, H
                        case 0x6D: cpu.registers.l = cpu.registers.l; INCR(1); // LD L, L
                        case 0x6E: cpu.registers.l = cpu.memory[cpu.registers.hl]; INCR(1); // LD L, (HL)
                        case 0x6F: cpu.registers.l = cpu.registers.a; INCR(1); // LD L, A

            // --- LD (HL), r ---
                        case 0x70: cpu.memory[cpu.registers.hl] = cpu.registers.b; INCR(1); // LD (HL), B
                        case 0x71: cpu.memory[cpu.registers.hl] = cpu.registers.c; INCR(1); // LD (HL), C
                        case 0x72: cpu.memory[cpu.registers.hl] = cpu.registers.d; INCR(1); // LD (HL), D
                        case 0x73: cpu.memory[cpu.registers.hl] = cpu.registers.e; INCR(1); // LD (HL), E
                        case 0x74: cpu.memory[cpu.registers.hl] = cpu.registers.h; INCR(1); // LD (HL), H
                        case 0x75: cpu.memory[cpu.registers.hl] = cpu.registers.l; INCR(1); // LD (HL), L
                        case 0x76: INCR(1); // HALT
                        case 0x77: cpu.memory[cpu.registers.hl] = cpu.registers.a; INCR(1); // LD (HL), A

            // --- LD A, r ---
                        case 0x78: cpu.registers.a = cpu.registers.b; INCR(1); // LD A, B
                        case 0x79: cpu.registers.a = cpu.registers.c; INCR(1); // LD A, C
                        case 0x7A: cpu.registers.a = cpu.registers.d; INCR(1); // LD A, D
                        case 0x7B: cpu.registers.a = cpu.registers.e; INCR(1); // LD A, E
                        case 0x7C: cpu.registers.a = cpu.registers.h; INCR(1); // LD A, H
                        case 0x7D: cpu.registers.a = cpu.registers.l; INCR(1); // LD A, L
                        case 0x7E: cpu.registers.a = cpu.memory[cpu.registers.hl]; INCR(1); // LD A, (HL)
                        case 0x7F: cpu.registers.a = cpu.registers.a; INCR(1); // LD A, A

            // --- ADD A, r ---
                        case 0x80: cpu_add(&cpu, cpu.registers.b); INCR(1); // ADD A, B
                        case 0x81: cpu_add(&cpu, cpu.registers.c); INCR(1); // ADD A, C
                        case 0x82: cpu_add(&cpu, cpu.registers.d); INCR(1); // ADD A, D
                        case 0x83: cpu_add(&cpu, cpu.registers.e); INCR(1); // ADD A, E
                        case 0x84: cpu_add(&cpu, cpu.registers.h); INCR(1); // ADD A, H
                        case 0x85: cpu_add(&cpu, cpu.registers.l); INCR(1); // ADD A, L
                        case 0x86: { // ADD A, (HL)
                uint8_t val = cpu.memory[cpu.registers.hl];
                cpu_add(&cpu, val);
                INCR(1);
            }
                        case 0x87: cpu_add(&cpu, cpu.registers.a); INCR(1); // ADD A, A

                        case 0x88: cpu_adc(&cpu, cpu.registers.b); INCR(1); // ADC A, B
                        case 0x89: cpu_adc(&cpu, cpu.registers.c); INCR(1); // ADC A, C
                        case 0x8A: cpu_adc(&cpu, cpu.registers.d); INCR(1); // ADC A, D
                        case 0x8B: cpu_adc(&cpu, cpu.registers.e); INCR(1); // ADC A, E
                        case 0x8C: cpu_adc(&cpu, cpu.registers.h); INCR(1); // ADC A, H
                        case 0x8D: cpu_adc(&cpu, cpu.registers.l); INCR(1); // ADC A, L
                        case 0x8E: { // ADC A, (HL)
                uint8_t val = cpu.memory[cpu.registers.hl];
                cpu_adc(&cpu, val);
                INCR(1);
            }
                        case 0x8F: cpu_adc(&cpu, cpu.registers.a); INCR(1);

            // --- SUB A, r (Subtract) ---
                        case 0x90: cpu_sub(&cpu, cpu.registers.b); INCR(1); // SUB B
                        case 0x91: cpu_sub(&cpu, cpu.registers.c); INCR(1); // SUB C
                        case 0x92: cpu_sub(&cpu, cpu.registers.d); INCR(1); // SUB D
                        case 0x93: cpu_sub(&cpu, cpu.registers.e); INCR(1); // SUB E
                        case 0x94: cpu_sub(&cpu, cpu.registers.h); INCR(1); // SUB H
                        case 0x95: cpu_sub(&cpu, cpu.registers.l); INCR(1); // SUB L
                        case 0x96: { // SUB (HL)
                uint8_t val = cpu.memory[cpu.registers.hl];
                cpu_sub(&cpu, val);
                INCR(1);
            }
                        case 0x97: cpu_sub(&cpu, cpu.registers.a); INCR(1); // SUB A

            // --- SBC A, r (Subtract with Carry) ---
                        case 0x98: cpu_sbc(&cpu, cpu.registers.b); INCR(1); // SBC A, B
                        case 0x99: cpu_sbc(&cpu, cpu.registers.c); INCR(1); // SBC A, C
                        case 0x9A: cpu_sbc(&cpu, cpu.registers.d); INCR(1); // SBC A, D
                        case 0x9B: cpu_sbc(&cpu, cpu.registers.e); INCR(1); // SBC A, E
                        case 0x9C: cpu_sbc(&cpu, cpu.registers.h); INCR(1); // SBC A, H
                        case 0x9D: cpu_sbc(&cpu, cpu.registers.l); INCR(1); // SBC A, L
                        case 0x9E: { // SBC A, (HL)
                uint8_t val = cpu.memory[cpu.registers.hl];
                cpu_sbc(&cpu, val);
                INCR(1);
            }
                        case 0x9F: cpu_sbc(&cpu, cpu.registers.a); INCR(1); // SBC A, A

            // --- AND A, r ---
                        case 0xA0: cpu_and(&cpu, cpu.registers.b); INCR(1);
                        case 0xA1: cpu_and(&cpu, cpu.registers.c); INCR(1);
                        case 0xA2: cpu_and(&cpu, cpu.registers.d); INCR(1);
                        case 0xA3: cpu_and(&cpu, cpu.registers.e); INCR(1);
                        case 0xA4: cpu_and(&cpu, cpu.registers.h); INCR(1);
                        case 0xA5: cpu_and(&cpu, cpu.registers.l); INCR(1);
                        case 0xA6: { // AND (HL)
                uint8_t val = cpu.memory[cpu.registers.hl];
                cpu_and(&cpu, val);
                INCR(1);
            }
                        case 0xA7: cpu_and(&cpu, cpu.registers.a); INCR(1);

            // --- XOR A, r ---
                        case 0xA8: cpu_xor(&cpu, cpu.registers.b); INCR(1);
                        case 0xA9: cpu_xor(&cpu, cpu.registers.c); INCR(1);
                        case 0xAA: cpu_xor(&cpu, cpu.registers.d); INCR(1);
                        case 0xAB: cpu_xor(&cpu, cpu.registers.e); INCR(1);
                        case 0xAC: cpu_xor(&cpu, cpu.registers.h); INCR(1);
                        case 0xAD: cpu_xor(&cpu, cpu.registers.l); INCR(1);
                        case 0xAE: { // XOR (HL)
                uint8_t val = cpu.memory[cpu.registers.hl];
                cpu_xor(&cpu, val);
                INCR(1);
            }
                        case 0xAF: { // XOR A
                cpu.registers.a ^= cpu.registers.a;
                cpu.registers.f = FLAG_Z;
                INCR(1);
            }

            // --- OR A, r ---
                        case 0xB0: cpu_or(&cpu, cpu.registers.b); INCR(1);
                        case 0xB1: { // OR A, C (Attention doublon B1) - Corrigé
                cpu_or(&cpu, cpu.registers.c); 
                INCR(1);
            }
                        case 0xB2: cpu_or(&cpu, cpu.registers.d); INCR(1);
                        case 0xB3: cpu_or(&cpu, cpu.registers.e); INCR(1);
                        case 0xB4: cpu_or(&cpu, cpu.registers.h); INCR(1);
                        case 0xB5: cpu_or(&cpu, cpu.registers.l); INCR(1);
                        case 0xB6: { // OR (HL)
                uint8_t val = cpu.memory[cpu.registers.hl];
                cpu_or(&cpu, val);
                INCR(1);
            }
                        case 0xB7: cpu_or(&cpu, cpu.registers.a); INCR(1);

            // --- CP A, r (Compare) ---
                        case 0xB8: cpu_cp(&cpu, cpu.registers.b); INCR(1); // CP B
                        case 0xB9: cpu_cp(&cpu, cpu.registers.c); INCR(1); // CP C
                        case 0xBA: cpu_cp(&cpu, cpu.registers.d); INCR(1); // CP D
                        case 0xBB: cpu_cp(&cpu, cpu.registers.e); INCR(1); // CP E
                        case 0xBC: cpu_cp(&cpu, cpu.registers.h); INCR(1); // CP H
                        case 0xBD: cpu_cp(&cpu, cpu.registers.l); INCR(1); // CP L
                        case 0xBE: { // CP (HL)
                uint8_t val = cpu.memory[cpu.registers.hl];
                cpu_cp(&cpu, val);
                INCR(1);
            }
                        case 0xBF: cpu_cp(&cpu, cpu.registers.a); INCR(1); // CP A

                        case 0xC1: { // POP BC
                cpu.registers.bc = stack_pop16(&cpu);
                INCR(1);
            }

                        case 0xC3: { // JP a16
                uint16_t destination = read_16(&cpu, cpu.pc + 1);
                cpu.pc = destination;
                break;
            }

                        case 0xC5: { // PUSH BC
                stack_push16(&cpu, cpu.registers.bc);
                INCR(1);
            }

                        case 0xC6: { // ADD A, u8 (Immediate)
                uint8_t val = cpu.memory[cpu.pc + 1];
                cpu_add(&cpu, val);
                INCR(2);
            }

                        case 0xC7: { // RST 00H
                stack_push16(&cpu, cpu.pc + 1);
                cpu.pc = 0x0000;
                break;
            }

                        case 0xC9: { // RET
                cpu.pc = stack_pop16(&cpu);
                break;
            }

            // MAIN OPCODES
        }
    }
    return SUCCESS;
}
