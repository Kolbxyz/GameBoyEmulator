#include <stdio.h>
#include <string.h>
#include "cpu.h"

static char sav_path[512];
static char state_path[512];
static uint32_t ram_size_for_cart = 0;

static uint32_t get_ram_size(uint8_t ram_byte)
{
    switch (ram_byte) {
        case 0x01: return 2048;
        case 0x02: return 8192;
        case 0x03: return 32768;
        case 0x04: return 131072;
        case 0x05: return 65536;
        default:   return 0;
    }
}

static int cart_has_battery(uint8_t type)
{
    return type == 0x03 || type == 0x06 || type == 0x09 ||
        type == 0x0D || type == 0x0F || type == 0x10 ||
        type == 0x13 || type == 0x1B || type == 0x1E ||
        type == 0x22 || type == 0xFF;
}

void init_save(const char *rom_path, cpu_t *cpu)
{
    ram_size_for_cart = get_ram_size(cpu->rom[0x0149]);

    // Build .sav path from ROM path
    strncpy(sav_path, rom_path, sizeof(sav_path) - 5);
    char *dot = strrchr(sav_path, '.');
    if (dot) *dot = '\0';
    strcat(sav_path, ".sav");

    // Build .state path
    strncpy(state_path, rom_path, sizeof(state_path) - 7);
    dot = strrchr(state_path, '.');
    if (dot) *dot = '\0';
    strcat(state_path, ".state");

    // Load existing save
    if (cart_has_battery(cpu->cartridge_type) && cpu->external_ram && ram_size_for_cart > 0) {
        FILE *f = fopen(sav_path, "rb");
        if (f) {
            fread(cpu->external_ram, 1, ram_size_for_cart, f);
            fclose(f);
        }
    }
}

void write_save(cpu_t *cpu)
{
    if (!cart_has_battery(cpu->cartridge_type) || !cpu->external_ram || ram_size_for_cart == 0)
        return;
    FILE *f = fopen(sav_path, "wb");
    if (f) {
        fwrite(cpu->external_ram, 1, ram_size_for_cart, f);
        fclose(f);
    }
}

// Save state: dump the entire CPU struct + external RAM
void save_state(cpu_t *cpu)
{
    FILE *f = fopen(state_path, "wb");
    if (!f) return;

    fwrite(cpu, sizeof(cpu_t), 1, f);
    if (cpu->external_ram && ram_size_for_cart > 0)
        fwrite(cpu->external_ram, 1, ram_size_for_cart, f);

    fclose(f);
}

void load_state(cpu_t *cpu)
{
    FILE *f = fopen(state_path, "rb");
    if (!f) return;

    // Save pointers that can't be serialized
    uint8_t *rom = cpu->rom;
    uint32_t rom_size = cpu->rom_size;
    uint8_t *ext_ram = cpu->external_ram;

    fread(cpu, sizeof(cpu_t), 1, f);

    // Restore pointers
    cpu->rom = rom;
    cpu->rom_size = rom_size;
    cpu->external_ram = ext_ram;

    if (cpu->external_ram && ram_size_for_cart > 0)
        fread(cpu->external_ram, 1, ram_size_for_cart, f);

    fclose(f);
}
