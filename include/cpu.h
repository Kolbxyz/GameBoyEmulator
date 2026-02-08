#include <stdint.h>

#ifndef CPU_H
    #define CPU_H
// --- [ MACROS ] ---
    #define MEMORY_SIZE 65536
    #define THROW(msg, code) throw_error(msg, code, __FILE__, __LINE__)

    #define INCR(x) { cpu.pc += (x); break; }
    #define OP_LD_16(opcode, reg) \
    case opcode: \
        cpu.registers.reg = read_16(&cpu, cpu.pc + 1); \
        INCR(3);
    #define OP_LD_8(opcode, reg) \
    case opcode: \
        cpu.registers.reg = read_8(&cpu, cpu.pc + 1); \
        INCR(2);

// --- [ ENUMS ] ---
typedef enum e_error {
    SUCCESS,
    INVALID_FILE,
    UNKNOWN_OPCODE
} error_t;

typedef enum e_flag {
    FLAG_Z = (1 << 7),
    FLAG_N = (1 << 6),
    FLAG_H = (1 << 5),
    FLAG_C = (1 << 4)

} flag_t;

// --- [ STRUCTS ] ---
typedef struct cpu_s {
    uint8_t memory[MEMORY_SIZE];
    uint16_t pc;
    uint16_t sp;

    uint8_t ime_scheduled;
    uint8_t ime;

    struct {
        union {
            // f: Flags (Z, N, H, C)
            // a: Accumulateur
            struct { uint8_t f; uint8_t a; };
            uint16_t af;
        };

        union {
            // Byte Counter / General Purpose
            struct { uint8_t c; uint8_t b; };
            uint16_t bc;
        };

        union {
            // Destination Address / Data
            // DE: dest ptr
            struct { uint8_t e; uint8_t d; };
            uint16_t de;
        };

        union {
            // Universal ptr
            // HL: source ptr
            struct { uint8_t l; uint8_t h; };
            uint16_t hl;
        };
    } registers;
} cpu_t;

// --- [ FUNCTIONS ] ---
void throw_error(char *msg, error_t code, char *FILE, int LINE);
void read_rom(const char *path, cpu_t *cpu);

// --- [ UTILS ] ---
uint8_t read_8(cpu_t *cpu, uint16_t addr);
uint16_t read_16(cpu_t *cpu, uint16_t addr);

// --- [ CPU ] ---
void cpu_add(cpu_t *cpu, uint8_t value);
void cpu_add_hl(cpu_t *cpu, uint16_t val);
void cpu_adc(cpu_t *cpu, uint8_t val);
void cpu_dec(cpu_t *cpu, uint8_t *reg);
void cpu_sbc(cpu_t *cpu, uint8_t val);
void cpu_inc(cpu_t *cpu, uint8_t *reg);
void cpu_cp(cpu_t *cpu, uint8_t val);
void cpu_sub(cpu_t *cpu, uint8_t val);
void cpu_and(cpu_t *cpu, uint8_t val);
void cpu_xor(cpu_t *cpu, uint8_t val);
void cpu_or(cpu_t *cpu, uint8_t val);
// STACK
void stack_push16(cpu_t *cpu, uint16_t value);
uint16_t stack_pop16(cpu_t *cpu);

#endif