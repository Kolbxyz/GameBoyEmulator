#include <stdint.h>

#ifndef CPU_H
    #define CPU_H

    #define MEMORY_SIZE 65536
    #define THROW(msg, code) throw_error(msg, code, __FILE__, __LINE__)

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

typedef struct cpu_s {
    uint8_t memory[MEMORY_SIZE];
    uint8_t *rom;
    uint32_t rom_size;

    uint8_t cartridge_type;
    uint16_t mbc1_bank_low;
    uint8_t mbc1_bank_high;
    uint8_t mbc1_rom_bank_high;
    uint8_t ram_enabled;
    uint8_t banking_mode;
    uint8_t *external_ram;

    uint16_t pc;
    uint16_t sp;

    uint8_t ime_scheduled;
    uint8_t ime;
    uint8_t halted;

    uint8_t joypad_state;

    // CGB support
    uint8_t cgb_mode;
    uint8_t bg_palette_data[64];   // 8 palettes × 4 colors × 2 bytes
    uint8_t obj_palette_data[64];
    uint8_t bcps;                  // BG palette index register
    uint8_t ocps;                  // OBJ palette index register
    uint8_t vram_bank;             // 0 or 1
    uint8_t vram_banks[2][0x2000]; // 2 VRAM banks (bank 0 mirrors memory[0x8000])
    uint8_t wram_bank;             // 1-7
    uint8_t wram_banks[8][0x1000]; // 8 WRAM banks (bank 0 at 0xC000, 1-7 switchable at 0xD000)

    struct {
        union { struct { uint8_t f; uint8_t a; }; uint16_t af; };
        union { struct { uint8_t c; uint8_t b; }; uint16_t bc; };
        union { struct { uint8_t e; uint8_t d; }; uint16_t de; };
        union { struct { uint8_t l; uint8_t h; }; uint16_t hl; };
    } registers;
} cpu_t;

void throw_error(char *msg, error_t code, char *FILE, int LINE);
void read_rom(const char *path, cpu_t *cpu);

uint8_t read_8(cpu_t *cpu, uint16_t addr);
void write_8(cpu_t *cpu, uint16_t addr, uint8_t val);
uint16_t read_16(cpu_t *cpu, uint16_t addr);
void write_16(cpu_t *cpu, uint16_t addr, uint16_t val);

int execute_instruction(cpu_t *cpu);

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
uint8_t cpu_rr(cpu_t *cpu, uint8_t val);
uint8_t cpu_srl(cpu_t *cpu, uint8_t val);
uint8_t cpu_rlc(cpu_t *cpu, uint8_t val);
uint8_t cpu_rrc(cpu_t *cpu, uint8_t val);
uint8_t cpu_rl(cpu_t *cpu, uint8_t val);
uint8_t cpu_sla(cpu_t *cpu, uint8_t val);
uint8_t cpu_sra(cpu_t *cpu, uint8_t val);
uint8_t cpu_swap(cpu_t *cpu, uint8_t val);
void cpu_bit(cpu_t *cpu, uint8_t bit, uint8_t val);
uint8_t cpu_res(uint8_t bit, uint8_t val);
uint8_t cpu_set(uint8_t bit, uint8_t val);

void update_graphics(cpu_t *cpu, int cycles);
void render_background(cpu_t *cpu, uint32_t *pixels);
void render_window(cpu_t *cpu, uint32_t *pixels);
void render_sprites(cpu_t *cpu, uint32_t *pixels);

void init_display(void);
void handle_interrupts(cpu_t *cpu);
void update_display(cpu_t *cpu);
void update_input(cpu_t *cpu);

void update_timers(cpu_t *cpu, int cycles);

void stack_push16(cpu_t *cpu, uint16_t value);
uint16_t stack_pop16(cpu_t *cpu);

void init_apu(void);
void update_audio(int cycles);
void apu_write(cpu_t *cpu, uint16_t addr, uint8_t val);
void cleanup_apu(void);

void init_save(const char *rom_path, cpu_t *cpu);
void write_save(cpu_t *cpu);
void save_state(cpu_t *cpu);
void load_state(cpu_t *cpu);

#endif