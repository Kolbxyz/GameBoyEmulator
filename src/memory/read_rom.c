#include "cpu.h"
#include <stdlib.h>
#include <stdio.h>

void read_rom(const char *path, cpu_t *cpu)
{
    FILE *f = fopen(path, "rb");

    if (f == NULL)
        THROW("Couldn't read that file.", INVALID_FILE);

    fseek(f, 0, SEEK_END);
    cpu->rom_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    cpu->rom = malloc(cpu->rom_size);
    if (!cpu->rom)
        THROW("Failed to allocate memory for ROM", INVALID_FILE);

    fread(cpu->rom, 1, cpu->rom_size, f);
    fclose(f);

    // Initial memory map (Bank 0)
    for (int i = 0; i < 0x8000 && i < cpu->rom_size; i++) {
        cpu->memory[i] = cpu->rom[i];
    }

    cpu->cartridge_type = cpu->rom[0x0147];
    cpu->mbc1_bank_low = 1;
    cpu->mbc1_bank_high = 0;
    cpu->mbc1_rom_bank_high = 0;
    cpu->ram_enabled = 0;
    cpu->banking_mode = 0;

    // Detect CGB mode
    uint8_t cgb_flag = cpu->rom[0x0143];
    cpu->cgb_mode = (cgb_flag == 0x80 || cgb_flag == 0xC0) ? 1 : 0;
    cpu->vram_bank = 0;
    cpu->wram_bank = 1;

    // Allocate external RAM if needed
    uint8_t ram_size_byte = cpu->rom[0x0149];
    uint32_t ram_size = 0;
    switch (ram_size_byte) {
        case 0x01: ram_size = 2048; break; // 2KB
        case 0x02: ram_size = 8192; break; // 8KB
        case 0x03: ram_size = 32768; break; // 32KB
        case 0x04: ram_size = 131072; break; // 128KB
        case 0x05: ram_size = 65536; break; // 64KB
    }
    if (ram_size > 0) {
        cpu->external_ram = calloc(1, ram_size);
    } else {
        cpu->external_ram = NULL;
    }
}