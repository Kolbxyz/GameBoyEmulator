#include <stddef.h>
#include "cpu.h"

static void execute_cb(cpu_t *cpu, uint8_t opcode)
{
    uint8_t *r[] = {
        &cpu->registers.b, &cpu->registers.c,
        &cpu->registers.d, &cpu->registers.e,
        &cpu->registers.h, &cpu->registers.l,
        NULL, &cpu->registers.a
    };
    int i = opcode & 7;
    uint8_t v = (i == 6) ? read_8(cpu, cpu->registers.hl) : *r[i];
    uint8_t res = 0;

    switch (opcode >> 3) {
        case 0: res = cpu_rlc(cpu, v); break;
        case 1: res = cpu_rrc(cpu, v); break;
        case 2: res = cpu_rl(cpu, v); break;
        case 3: res = cpu_rr(cpu, v); break;
        case 4: res = cpu_sla(cpu, v); break;
        case 5: res = cpu_sra(cpu, v); break;
        case 6: res = cpu_swap(cpu, v); break;
        case 7: res = cpu_srl(cpu, v); break;
        case 8 ... 15: cpu_bit(cpu, (opcode >> 3) & 7, v); return;
        case 16 ... 23: res = cpu_res((opcode >> 3) & 7, v); break;
        case 24 ... 31: res = cpu_set((opcode >> 3) & 7, v); break;
    }
    if (i == 6)
        write_8(cpu, cpu->registers.hl, res);
    else
        *r[i] = res;
}

int execute_instruction(cpu_t *cpu)
{
    uint8_t op = read_8(cpu, cpu->pc);
    int c = 4;

    switch (op) {

    // -- 0x00: NOP --
    case 0x00: cpu->pc++; break;

    // -- 0x01-0x0F --
    case 0x01: cpu->registers.bc = read_16(cpu, cpu->pc + 1); cpu->pc += 3; c = 12; break;
    case 0x02: write_8(cpu, cpu->registers.bc, cpu->registers.a); cpu->pc++; c = 8; break;
    case 0x03: cpu->registers.bc++; cpu->pc++; c = 8; break;
    case 0x04: cpu_inc(cpu, &cpu->registers.b); cpu->pc++; break;
    case 0x05: cpu_dec(cpu, &cpu->registers.b); cpu->pc++; break;
    case 0x06: cpu->registers.b = read_8(cpu, cpu->pc + 1); cpu->pc += 2; c = 8; break;
    case 0x07: {
        uint8_t a = cpu->registers.a, cy = a >> 7;
        cpu->registers.a = (a << 1) | cy;
        cpu->registers.f = cy << 4;
        cpu->pc++; break;
    }
    case 0x08: {
        uint16_t addr = read_16(cpu, cpu->pc + 1);
        write_8(cpu, addr, cpu->sp & 0xFF);
        write_8(cpu, addr + 1, cpu->sp >> 8);
        cpu->pc += 3; c = 20; break;
    }
    case 0x09: cpu_add_hl(cpu, cpu->registers.bc); cpu->pc++; c = 8; break;
    case 0x0A: cpu->registers.a = read_8(cpu, cpu->registers.bc); cpu->pc++; c = 8; break;
    case 0x0B: cpu->registers.bc--; cpu->pc++; c = 8; break;
    case 0x0C: cpu_inc(cpu, &cpu->registers.c); cpu->pc++; break;
    case 0x0D: cpu_dec(cpu, &cpu->registers.c); cpu->pc++; break;
    case 0x0E: cpu->registers.c = read_8(cpu, cpu->pc + 1); cpu->pc += 2; c = 8; break;
    case 0x0F: {
        uint8_t a = cpu->registers.a, cy = a & 1;
        cpu->registers.a = (a >> 1) | (cy << 7);
        cpu->registers.f = cy << 4;
        cpu->pc++; break;
    }

    // -- 0x10-0x1F --
    case 0x10: cpu->pc += 2; break;
    case 0x11: cpu->registers.de = read_16(cpu, cpu->pc + 1); cpu->pc += 3; c = 12; break;
    case 0x12: write_8(cpu, cpu->registers.de, cpu->registers.a); cpu->pc++; c = 8; break;
    case 0x13: cpu->registers.de++; cpu->pc++; c = 8; break;
    case 0x14: cpu_inc(cpu, &cpu->registers.d); cpu->pc++; break;
    case 0x15: cpu_dec(cpu, &cpu->registers.d); cpu->pc++; break;
    case 0x16: cpu->registers.d = read_8(cpu, cpu->pc + 1); cpu->pc += 2; c = 8; break;
    case 0x17: {
        uint8_t a = cpu->registers.a;
        uint8_t oc = (cpu->registers.f & FLAG_C) ? 1 : 0;
        cpu->registers.a = (a << 1) | oc;
        cpu->registers.f = (a >> 7) << 4;
        cpu->pc++; break;
    }
    case 0x18: { int8_t o = (int8_t)read_8(cpu, cpu->pc + 1); cpu->pc += 2 + o; c = 12; break; }
    case 0x19: cpu_add_hl(cpu, cpu->registers.de); cpu->pc++; c = 8; break;
    case 0x1A: cpu->registers.a = read_8(cpu, cpu->registers.de); cpu->pc++; c = 8; break;
    case 0x1B: cpu->registers.de--; cpu->pc++; c = 8; break;
    case 0x1C: cpu_inc(cpu, &cpu->registers.e); cpu->pc++; break;
    case 0x1D: cpu_dec(cpu, &cpu->registers.e); cpu->pc++; break;
    case 0x1E: cpu->registers.e = read_8(cpu, cpu->pc + 1); cpu->pc += 2; c = 8; break;
    case 0x1F: {
        uint8_t a = cpu->registers.a;
        uint8_t oc = (cpu->registers.f & FLAG_C) ? 1 : 0;
        cpu->registers.a = (a >> 1) | (oc << 7);
        cpu->registers.f = (a & 1) << 4;
        cpu->pc++; break;
    }

    // -- 0x20-0x2F --
    case 0x20: {
        int8_t o = (int8_t)read_8(cpu, cpu->pc + 1); cpu->pc += 2;
        if (!(cpu->registers.f & FLAG_Z)) { cpu->pc += o; c = 12; } else c = 8;
        break;
    }
    case 0x21: cpu->registers.hl = read_16(cpu, cpu->pc + 1); cpu->pc += 3; c = 12; break;
    case 0x22: write_8(cpu, cpu->registers.hl++, cpu->registers.a); cpu->pc++; c = 8; break;
    case 0x23: cpu->registers.hl++; cpu->pc++; c = 8; break;
    case 0x24: cpu_inc(cpu, &cpu->registers.h); cpu->pc++; break;
    case 0x25: cpu_dec(cpu, &cpu->registers.h); cpu->pc++; break;
    case 0x26: cpu->registers.h = read_8(cpu, cpu->pc + 1); cpu->pc += 2; c = 8; break;
    case 0x27: {
        uint8_t u = 0;
        if ((cpu->registers.f & FLAG_H) || (!(cpu->registers.f & FLAG_N) && (cpu->registers.a & 0xF) > 9))
            u = 6;
        if ((cpu->registers.f & FLAG_C) || (!(cpu->registers.f & FLAG_N) && cpu->registers.a > 0x99)) {
            u |= 0x60;
            cpu->registers.f |= FLAG_C;
        }
        cpu->registers.a += (cpu->registers.f & FLAG_N) ? -u : u;
        cpu->registers.f &= ~(FLAG_H | FLAG_Z);
        if (cpu->registers.a == 0) cpu->registers.f |= FLAG_Z;
        cpu->pc++; break;
    }
    case 0x28: {
        int8_t o = (int8_t)read_8(cpu, cpu->pc + 1); cpu->pc += 2;
        if (cpu->registers.f & FLAG_Z) { cpu->pc += o; c = 12; } else c = 8;
        break;
    }
    case 0x29: cpu_add_hl(cpu, cpu->registers.hl); cpu->pc++; c = 8; break;
    case 0x2A: cpu->registers.a = read_8(cpu, cpu->registers.hl++); cpu->pc++; c = 8; break;
    case 0x2B: cpu->registers.hl--; cpu->pc++; c = 8; break;
    case 0x2C: cpu_inc(cpu, &cpu->registers.l); cpu->pc++; break;
    case 0x2D: cpu_dec(cpu, &cpu->registers.l); cpu->pc++; break;
    case 0x2E: cpu->registers.l = read_8(cpu, cpu->pc + 1); cpu->pc += 2; c = 8; break;
    case 0x2F: cpu->registers.a = ~cpu->registers.a; cpu->registers.f |= 0x60; cpu->pc++; break;

    // -- 0x30-0x3F --
    case 0x30: {
        int8_t o = (int8_t)read_8(cpu, cpu->pc + 1); cpu->pc += 2;
        if (!(cpu->registers.f & FLAG_C)) { cpu->pc += o; c = 12; } else c = 8;
        break;
    }
    case 0x31: cpu->sp = read_16(cpu, cpu->pc + 1); cpu->pc += 3; c = 12; break;
    case 0x32: write_8(cpu, cpu->registers.hl--, cpu->registers.a); cpu->pc++; c = 8; break;
    case 0x33: cpu->sp++; cpu->pc++; c = 8; break;
    case 0x34: {
        uint8_t v = read_8(cpu, cpu->registers.hl);
        cpu_inc(cpu, &v);
        write_8(cpu, cpu->registers.hl, v);
        cpu->pc++; c = 12; break;
    }
    case 0x35: {
        uint8_t v = read_8(cpu, cpu->registers.hl);
        cpu_dec(cpu, &v);
        write_8(cpu, cpu->registers.hl, v);
        cpu->pc++; c = 12; break;
    }
    case 0x36: write_8(cpu, cpu->registers.hl, read_8(cpu, cpu->pc + 1)); cpu->pc += 2; c = 12; break;
    case 0x37: cpu->registers.f = (cpu->registers.f & FLAG_Z) | FLAG_C; cpu->pc++; break;
    case 0x38: {
        int8_t o = (int8_t)read_8(cpu, cpu->pc + 1); cpu->pc += 2;
        if (cpu->registers.f & FLAG_C) { cpu->pc += o; c = 12; } else c = 8;
        break;
    }
    case 0x39: cpu_add_hl(cpu, cpu->sp); cpu->pc++; c = 8; break;
    case 0x3A: cpu->registers.a = read_8(cpu, cpu->registers.hl--); cpu->pc++; c = 8; break;
    case 0x3B: cpu->sp--; cpu->pc++; c = 8; break;
    case 0x3C: cpu_inc(cpu, &cpu->registers.a); cpu->pc++; break;
    case 0x3D: cpu_dec(cpu, &cpu->registers.a); cpu->pc++; break;
    case 0x3E: cpu->registers.a = read_8(cpu, cpu->pc + 1); cpu->pc += 2; c = 8; break;
    case 0x3F: {
        uint8_t c_ = (cpu->registers.f & FLAG_C) ? 0 : FLAG_C;
        cpu->registers.f = (cpu->registers.f & FLAG_Z) | c_;
        cpu->pc++; break;
    }

    // -- 0x40-0x7F: LD r,r and HALT --
    case 0x40 ... 0x75: case 0x77 ... 0x7F: {
        uint8_t *r[] = {
            &cpu->registers.b, &cpu->registers.c,
            &cpu->registers.d, &cpu->registers.e,
            &cpu->registers.h, &cpu->registers.l,
            NULL, &cpu->registers.a
        };
        int d = (op >> 3) & 7, s = op & 7;
        if (d == 6)      { write_8(cpu, cpu->registers.hl, *r[s]); c = 8; }
        else if (s == 6)  { *r[d] = read_8(cpu, cpu->registers.hl); c = 8; }
        else              { *r[d] = *r[s]; }
        cpu->pc++; break;
    }
    case 0x76: cpu->halted = 1; break;

    // -- 0x80-0xBF: ALU ops --
    case 0x80 ... 0xBF: {
        uint8_t *r[] = {
            &cpu->registers.b, &cpu->registers.c,
            &cpu->registers.d, &cpu->registers.e,
            &cpu->registers.h, &cpu->registers.l,
            NULL, &cpu->registers.a
        };
        uint8_t v = ((op & 7) == 6) ? read_8(cpu, cpu->registers.hl) : *r[op & 7];
        switch ((op >> 3) & 7) {
            case 0: cpu_add(cpu, v); break;
            case 1: cpu_adc(cpu, v); break;
            case 2: cpu_sub(cpu, v); break;
            case 3: cpu_sbc(cpu, v); break;
            case 4: cpu_and(cpu, v); break;
            case 5: cpu_xor(cpu, v); break;
            case 6: cpu_or(cpu, v); break;
            case 7: cpu_cp(cpu, v); break;
        }
        cpu->pc++;
        c = ((op & 7) == 6) ? 8 : 4; break;
    }

    // -- 0xC0-0xCF --
    case 0xC0:
        if (!(cpu->registers.f & FLAG_Z)) { cpu->pc = stack_pop16(cpu); c = 20; }
        else { cpu->pc++; c = 8; } break;
    case 0xC1: cpu->registers.bc = stack_pop16(cpu); cpu->pc++; c = 12; break;
    case 0xC2:
        if (!(cpu->registers.f & FLAG_Z)) { cpu->pc = read_16(cpu, cpu->pc + 1); c = 16; }
        else { cpu->pc += 3; c = 12; } break;
    case 0xC3: cpu->pc = read_16(cpu, cpu->pc + 1); c = 16; break;
    case 0xC4:
        if (!(cpu->registers.f & FLAG_Z)) { stack_push16(cpu, cpu->pc + 3); cpu->pc = read_16(cpu, cpu->pc + 1); c = 24; }
        else { cpu->pc += 3; c = 12; } break;
    case 0xC5: stack_push16(cpu, cpu->registers.bc); cpu->pc++; c = 16; break;
    case 0xC6: cpu_add(cpu, read_8(cpu, cpu->pc + 1)); cpu->pc += 2; c = 8; break;
    case 0xC7: stack_push16(cpu, cpu->pc + 1); cpu->pc = 0x0000; c = 16; break;
    case 0xC8:
        if (cpu->registers.f & FLAG_Z) { cpu->pc = stack_pop16(cpu); c = 20; }
        else { cpu->pc++; c = 8; } break;
    case 0xC9: cpu->pc = stack_pop16(cpu); c = 16; break;
    case 0xCA:
        if (cpu->registers.f & FLAG_Z) { cpu->pc = read_16(cpu, cpu->pc + 1); c = 16; }
        else { cpu->pc += 3; c = 12; } break;
    case 0xCB: execute_cb(cpu, read_8(cpu, cpu->pc + 1)); cpu->pc += 2; c = 8; break;
    case 0xCC:
        if (cpu->registers.f & FLAG_Z) { stack_push16(cpu, cpu->pc + 3); cpu->pc = read_16(cpu, cpu->pc + 1); c = 24; }
        else { cpu->pc += 3; c = 12; } break;
    case 0xCD: {
        uint16_t t = read_16(cpu, cpu->pc + 1);
        stack_push16(cpu, cpu->pc + 3); cpu->pc = t; c = 24; break;
    }
    case 0xCE: cpu_adc(cpu, read_8(cpu, cpu->pc + 1)); cpu->pc += 2; c = 8; break;
    case 0xCF: stack_push16(cpu, cpu->pc + 1); cpu->pc = 0x0008; c = 16; break;

    // -- 0xD0-0xDF --
    case 0xD0:
        if (!(cpu->registers.f & FLAG_C)) { cpu->pc = stack_pop16(cpu); c = 20; }
        else { cpu->pc++; c = 8; } break;
    case 0xD1: cpu->registers.de = stack_pop16(cpu); cpu->pc++; c = 12; break;
    case 0xD2:
        if (!(cpu->registers.f & FLAG_C)) { cpu->pc = read_16(cpu, cpu->pc + 1); c = 16; }
        else { cpu->pc += 3; c = 12; } break;
    case 0xD4:
        if (!(cpu->registers.f & FLAG_C)) { stack_push16(cpu, cpu->pc + 3); cpu->pc = read_16(cpu, cpu->pc + 1); c = 24; }
        else { cpu->pc += 3; c = 12; } break;
    case 0xD5: stack_push16(cpu, cpu->registers.de); cpu->pc++; c = 16; break;
    case 0xD6: cpu_sub(cpu, read_8(cpu, cpu->pc + 1)); cpu->pc += 2; c = 8; break;
    case 0xD7: stack_push16(cpu, cpu->pc + 1); cpu->pc = 0x0010; c = 16; break;
    case 0xD8:
        if (cpu->registers.f & FLAG_C) { cpu->pc = stack_pop16(cpu); c = 20; }
        else { cpu->pc++; c = 8; } break;
    case 0xD9: cpu->pc = stack_pop16(cpu); cpu->ime = 1; c = 16; break;
    case 0xDA:
        if (cpu->registers.f & FLAG_C) { cpu->pc = read_16(cpu, cpu->pc + 1); c = 16; }
        else { cpu->pc += 3; c = 12; } break;
    case 0xDC:
        if (cpu->registers.f & FLAG_C) { stack_push16(cpu, cpu->pc + 3); cpu->pc = read_16(cpu, cpu->pc + 1); c = 24; }
        else { cpu->pc += 3; c = 12; } break;
    case 0xDE: cpu_sbc(cpu, read_8(cpu, cpu->pc + 1)); cpu->pc += 2; c = 8; break;
    case 0xDF: stack_push16(cpu, cpu->pc + 1); cpu->pc = 0x0018; c = 16; break;

    // -- 0xE0-0xEF --
    case 0xE0: write_8(cpu, 0xFF00 + read_8(cpu, cpu->pc + 1), cpu->registers.a); cpu->pc += 2; c = 12; break;
    case 0xE1: cpu->registers.hl = stack_pop16(cpu); cpu->pc++; c = 12; break;
    case 0xE2: write_8(cpu, 0xFF00 + cpu->registers.c, cpu->registers.a); cpu->pc++; c = 8; break;
    case 0xE5: stack_push16(cpu, cpu->registers.hl); cpu->pc++; c = 16; break;
    case 0xE6:
        cpu->registers.a &= read_8(cpu, cpu->pc + 1);
        cpu->registers.f = (cpu->registers.a == 0 ? FLAG_Z : 0) | FLAG_H;
        cpu->pc += 2; c = 8; break;
    case 0xE7: stack_push16(cpu, cpu->pc + 1); cpu->pc = 0x0020; c = 16; break;
    case 0xE8: {
        int8_t o = (int8_t)read_8(cpu, cpu->pc + 1);
        cpu->registers.f = (((cpu->sp & 0xF) + (o & 0xF) > 0xF) << 5)
                         | (((cpu->sp & 0xFF) + (o & 0xFF) > 0xFF) << 4);
        cpu->sp += o;
        cpu->pc += 2; c = 16; break;
    }
    case 0xE9: cpu->pc = cpu->registers.hl; break;
    case 0xEA: write_8(cpu, read_16(cpu, cpu->pc + 1), cpu->registers.a); cpu->pc += 3; c = 16; break;
    case 0xEE: cpu_xor(cpu, read_8(cpu, cpu->pc + 1)); cpu->pc += 2; c = 8; break;
    case 0xEF: stack_push16(cpu, cpu->pc + 1); cpu->pc = 0x0028; c = 16; break;

    // -- 0xF0-0xFF --
    case 0xF0: cpu->registers.a = read_8(cpu, 0xFF00 + read_8(cpu, cpu->pc + 1)); cpu->pc += 2; c = 12; break;
    case 0xF1: cpu->registers.af = stack_pop16(cpu) & 0xFFF0; cpu->pc++; c = 12; break;
    case 0xF2: cpu->registers.a = read_8(cpu, 0xFF00 + cpu->registers.c); cpu->pc++; c = 8; break;
    case 0xF3: cpu->ime = 0; cpu->pc++; break;
    case 0xF5: stack_push16(cpu, cpu->registers.af); cpu->pc++; c = 16; break;
    case 0xF6: cpu_or(cpu, read_8(cpu, cpu->pc + 1)); cpu->pc += 2; c = 8; break;
    case 0xF7: stack_push16(cpu, cpu->pc + 1); cpu->pc = 0x0030; c = 16; break;
    case 0xF8: {
        int8_t o = (int8_t)read_8(cpu, cpu->pc + 1);
        cpu->registers.f = (((cpu->sp & 0xF) + (o & 0xF) > 0xF) << 5)
                         | (((cpu->sp & 0xFF) + (o & 0xFF) > 0xFF) << 4);
        cpu->registers.hl = cpu->sp + o;
        cpu->pc += 2; c = 12; break;
    }
    case 0xF9: cpu->sp = cpu->registers.hl; cpu->pc++; c = 8; break;
    case 0xFA: cpu->registers.a = read_8(cpu, read_16(cpu, cpu->pc + 1)); cpu->pc += 3; c = 16; break;
    case 0xFB: cpu->ime_scheduled = 2; cpu->pc++; break;
    case 0xFE: cpu_cp(cpu, read_8(cpu, cpu->pc + 1)); cpu->pc += 2; c = 8; break;
    case 0xFF: stack_push16(cpu, cpu->pc + 1); cpu->pc = 0x0038; c = 16; break;

    default: c = 4; cpu->pc++; break;
    }
    return c;
}
