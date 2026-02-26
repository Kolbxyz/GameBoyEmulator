#include "cpu.h"
#include <stdio.h>

uint8_t read_8(cpu_t *cpu, uint16_t address)
{
    if (address < 0x4000) {
        uint32_t bank = 0;
        if (cpu->banking_mode == 1 && cpu->cartridge_type <= 0x03) {
            bank = (cpu->mbc1_bank_high << 5);
        }
        uint32_t offset = (bank * 0x4000) + address;
        if (offset < cpu->rom_size)
            return cpu->rom[offset];
        return 0xFF;
    }

    if (address >= 0x4000 && address < 0x8000) {
        uint32_t bank = cpu->mbc1_bank_low;
        if (cpu->cartridge_type <= 0x03) {
            bank |= (cpu->mbc1_bank_high << 5);
        } else if (cpu->cartridge_type >= 0x19 && cpu->cartridge_type <= 0x1E) {
            bank |= (cpu->mbc1_rom_bank_high << 8);
        }
        uint32_t offset = (bank * 0x4000) + (address - 0x4000);
        if (offset < cpu->rom_size)
            return cpu->rom[offset];
        return 0xFF;
    }

    // VRAM — read from active bank in CGB mode
    if (address >= 0x8000 && address < 0xA000) {
        if (cpu->cgb_mode)
            return cpu->vram_banks[cpu->vram_bank][address - 0x8000];
        return cpu->memory[address];
    }

    // External RAM read
    if (address >= 0xA000 && address < 0xC000) {
        if (cpu->ram_enabled) {
            if (cpu->cartridge_type >= 0x0F && cpu->cartridge_type <= 0x13 && cpu->mbc1_bank_high >= 0x08)
                return 0x00;
            if (cpu->external_ram) {
                uint32_t bank = 0;
                if (cpu->cartridge_type >= 0x01 && cpu->cartridge_type <= 0x03) {
                    if (cpu->banking_mode == 1) bank = cpu->mbc1_bank_high;
                } else {
                    bank = cpu->mbc1_bank_high;
                }
                uint32_t offset = (bank * 0x2000) + (address - 0xA000);
                return cpu->external_ram[offset];
            }
        }
        return 0xFF;
    }

    // WRAM bank 0 (0xC000-0xCFFF)
    if (cpu->cgb_mode && address >= 0xC000 && address < 0xD000)
        return cpu->wram_banks[0][address - 0xC000];

    // WRAM switchable bank (0xD000-0xDFFF)
    if (cpu->cgb_mode && address >= 0xD000 && address < 0xE000)
        return cpu->wram_banks[cpu->wram_bank][address - 0xD000];

    if (address == 0xFF00) {
        uint8_t res = cpu->memory[0xFF00] | 0xCF;
        if (!(res & 0x10)) res &= ~(cpu->joypad_state & 0x0F);
        if (!(res & 0x20)) res &= ~((cpu->joypad_state >> 4) & 0x0F);
        return res;
    }

    // CGB palette register reads
    if (cpu->cgb_mode) {
        if (address == 0xFF68) return cpu->bcps;
        if (address == 0xFF69) return cpu->bg_palette_data[cpu->bcps & 0x3F];
        if (address == 0xFF6A) return cpu->ocps;
        if (address == 0xFF6B) return cpu->obj_palette_data[cpu->ocps & 0x3F];
        if (address == 0xFF4F) return cpu->vram_bank | 0xFE;
        if (address == 0xFF70) return cpu->wram_bank;
        if (address == 0xFF4D) return (cpu->double_speed << 7) | cpu->speed_switch_armed;
        if (address == 0xFF55) return cpu->hdma_active ? (cpu->hdma_remaining / 16 - 1) : 0xFF;
    }

    // APU register reads
    if (address >= 0xFF10 && address <= 0xFF3F)
        return apu_read(address);

    return cpu->memory[address];
}

void write_8(cpu_t *cpu, uint16_t address, uint8_t value)
{
    if (address < 0x2000) {
        cpu->ram_enabled = ((value & 0x0F) == 0x0A);
        return;
    }

    if (address >= 0x2000 && address < 0x4000) {
        if (cpu->cartridge_type <= 0x03) {
            cpu->mbc1_bank_low = value & 0x1F;
            if (cpu->mbc1_bank_low == 0) cpu->mbc1_bank_low = 1;
        } else if (cpu->cartridge_type >= 0x0F && cpu->cartridge_type <= 0x13) {
            cpu->mbc1_bank_low = value & 0x7F;
            if (cpu->mbc1_bank_low == 0) cpu->mbc1_bank_low = 1;
        } else if (cpu->cartridge_type >= 0x19 && cpu->cartridge_type <= 0x1E) {
            if (address < 0x3000) cpu->mbc1_bank_low = value;
            else cpu->mbc1_rom_bank_high = value & 0x01;
        }
        return;
    }

    if (address >= 0x4000 && address < 0x6000) {
        if (cpu->cartridge_type <= 0x03)
            cpu->mbc1_bank_high = value & 0x03;
        else if (cpu->cartridge_type >= 0x0F && cpu->cartridge_type <= 0x13)
            cpu->mbc1_bank_high = value & 0x0F;
        else if (cpu->cartridge_type >= 0x19 && cpu->cartridge_type <= 0x1E)
            cpu->mbc1_bank_high = value & 0x0F;
        return;
    }

    if (address >= 0x6000 && address < 0x8000) {
        if (cpu->cartridge_type <= 0x03)
            cpu->banking_mode = value & 0x01;
        return;
    }

    // External RAM write
    if (address >= 0xA000 && address < 0xC000) {
        if (cpu->ram_enabled && cpu->external_ram) {
            uint32_t bank = 0;
            if (cpu->cartridge_type >= 0x01 && cpu->cartridge_type <= 0x03) {
                if (cpu->banking_mode == 1) bank = cpu->mbc1_bank_high;
            } else {
                bank = cpu->mbc1_bank_high;
            }
            uint32_t offset = (bank * 0x2000) + (address - 0xA000);
            cpu->external_ram[offset] = value;
        }
        return;
    }

    if (address == 0xFF46) {
        uint16_t src = value << 8;
        for (int i = 0; i < 160; i++)
            cpu->memory[0xFE00 + i] = read_8(cpu, src + i);
        return;
    }

    // VRAM write — route to active bank in CGB mode
    if (address >= 0x8000 && address < 0xA000) {
        if (cpu->cgb_mode)
            cpu->vram_banks[cpu->vram_bank][address - 0x8000] = value;
        cpu->memory[address] = value;
        return;
    }

    // WRAM bank 0 (0xC000-0xCFFF)
    if (cpu->cgb_mode && address >= 0xC000 && address < 0xD000) {
        cpu->wram_banks[0][address - 0xC000] = value;
        cpu->memory[address] = value;
        return;
    }

    // WRAM switchable bank (0xD000-0xDFFF)
    if (cpu->cgb_mode && address >= 0xD000 && address < 0xE000) {
        cpu->wram_banks[cpu->wram_bank][address - 0xD000] = value;
        cpu->memory[address] = value;
        return;
    }

    // CGB palette registers
    if (cpu->cgb_mode) {
        if (address == 0xFF68) { cpu->bcps = value; return; }
        if (address == 0xFF69) {
            cpu->bg_palette_data[cpu->bcps & 0x3F] = value;
            if (cpu->bcps & 0x80) cpu->bcps = (cpu->bcps & 0x80) | ((cpu->bcps + 1) & 0x3F);
            return;
        }
        if (address == 0xFF6A) { cpu->ocps = value; return; }
        if (address == 0xFF6B) {
            cpu->obj_palette_data[cpu->ocps & 0x3F] = value;
            if (cpu->ocps & 0x80) cpu->ocps = (cpu->ocps & 0x80) | ((cpu->ocps + 1) & 0x3F);
            return;
        }
        if (address == 0xFF4F) { cpu->vram_bank = value & 0x01; return; }
        if (address == 0xFF70) {
            cpu->wram_bank = value & 0x07;
            if (cpu->wram_bank == 0) cpu->wram_bank = 1;
            return;
        }
        if (address == 0xFF4D) {
            cpu->speed_switch_armed = value & 0x01;
            return;
        }
        // HDMA registers
        if (address == 0xFF51) { cpu->hdma_src_hi = value; return; }
        if (address == 0xFF52) { cpu->hdma_src_lo = value & 0xF0; return; }
        if (address == 0xFF53) { cpu->hdma_dst_hi = value & 0x1F; return; }
        if (address == 0xFF54) { cpu->hdma_dst_lo = value & 0xF0; return; }
        if (address == 0xFF55) {
            if (cpu->hdma_active && !(value & 0x80)) {
                cpu->hdma_active = 0;
            } else {
                uint16_t src = (cpu->hdma_src_hi << 8) | cpu->hdma_src_lo;
                uint16_t dst = 0x8000 | ((cpu->hdma_dst_hi << 8) | cpu->hdma_dst_lo);
                uint16_t len = ((value & 0x7F) + 1) * 16;
                if (value & 0x80) {
                    // HDMA (HBlank DMA) — transfer 16 bytes per HBlank
                    cpu->hdma_active = 1;
                    cpu->hdma_remaining = len;
                } else {
                    // GDMA — immediate transfer
                    for (uint16_t i = 0; i < len; i++)
                        write_8(cpu, dst + i, read_8(cpu, src + i));
                    cpu->hdma_src_hi = (src + len) >> 8;
                    cpu->hdma_src_lo = (src + len) & 0xF0;
                    cpu->hdma_dst_hi = ((dst + len) >> 8) & 0x1F;
                    cpu->hdma_dst_lo = (dst + len) & 0xF0;
                    cpu->hdma_active = 0;
                }
            }
            return;
        }
    }

    if (address == 0xFF02) {
        if (value == 0x81) {
            putchar(cpu->memory[0xFF01]);
            fflush(stdout);
            cpu->serial_timer = 4096;
        }
        cpu->memory[0xFF02] = value;
        return;
    }

    if (address == 0xFF04) {
        cpu->div_counter = 0;
        cpu->memory[0xFF04] = 0;
        return;
    }

    // APU registers + wave RAM
    if ((address >= 0xFF10 && address <= 0xFF26) || (address >= 0xFF30 && address <= 0xFF3F)) {
        cpu->memory[address] = value;
        apu_write(cpu, address, value);
        return;
    }

    cpu->memory[address] = value;
}

uint16_t read_16(cpu_t *cpu, uint16_t address)
{
    return read_8(cpu, address) | (read_8(cpu, address + 1) << 8);
}

void write_16(cpu_t *cpu, uint16_t address, uint16_t value)
{
    write_8(cpu, address, value & 0xFF);
    write_8(cpu, address + 1, value >> 8);
}

void hdma_hblank_tick(cpu_t *cpu)
{
    if (!cpu->cgb_mode || !cpu->hdma_active || cpu->hdma_remaining == 0)
        return;
    uint16_t src = (cpu->hdma_src_hi << 8) | cpu->hdma_src_lo;
    uint16_t dst = 0x8000 | ((cpu->hdma_dst_hi << 8) | cpu->hdma_dst_lo);
    for (int i = 0; i < 16; i++)
        write_8(cpu, dst + i, read_8(cpu, src + i));
    src += 16;
    dst += 16;
    cpu->hdma_src_hi = src >> 8;
    cpu->hdma_src_lo = src & 0xF0;
    cpu->hdma_dst_hi = (dst >> 8) & 0x1F;
    cpu->hdma_dst_lo = dst & 0xF0;
    cpu->hdma_remaining -= 16;
    if (cpu->hdma_remaining == 0)
        cpu->hdma_active = 0;
}