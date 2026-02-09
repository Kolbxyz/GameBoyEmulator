#include "cpu.h"
#include <SDL2/SDL.h>

uint32_t palette[4] = {
    0xFFFFFFFF,
    0xFF8BAC0F,
    0xFF306230,
    0xFF0F380F
};

// Convert CGB 15-bit color (RGB555) to ARGB8888
static uint32_t cgb_to_argb(uint8_t lo, uint8_t hi)
{
    uint16_t rgb555 = lo | (hi << 8);
    uint8_t r = (rgb555 & 0x1F) << 3;
    uint8_t g = ((rgb555 >> 5) & 0x1F) << 3;
    uint8_t b = ((rgb555 >> 10) & 0x1F) << 3;
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

static uint32_t get_cgb_bg_color(cpu_t *cpu, uint8_t pal_num, uint8_t color_id)
{
    int idx = (pal_num * 8) + (color_id * 2);
    return cgb_to_argb(cpu->bg_palette_data[idx], cpu->bg_palette_data[idx + 1]);
}

static uint32_t get_cgb_obj_color(cpu_t *cpu, uint8_t pal_num, uint8_t color_id)
{
    int idx = (pal_num * 8) + (color_id * 2);
    return cgb_to_argb(cpu->obj_palette_data[idx], cpu->obj_palette_data[idx + 1]);
}

void render_background(cpu_t *cpu, uint32_t *pixels) {
    uint8_t scy = cpu->memory[0xFF42];
    uint8_t scx = cpu->memory[0xFF43];
    uint8_t lcdc = cpu->memory[0xFF40];

    if (!(lcdc & 0x80)) return;

    if (!(lcdc & 0x01) && !cpu->cgb_mode) {
        for (int i = 0; i < 160 * 144; i++) pixels[i] = palette[0];
        return;
    }

    uint16_t map_base = (lcdc & 0x08) ? 0x9C00 : 0x9800;
    uint16_t data_base = (lcdc & 0x10) ? 0x8000 : 0x8800;
    int is_signed = !(lcdc & 0x10);

    for (int y = 0; y < 144; y++) {
        for (int x = 0; x < 160; x++) {
            uint8_t bg_y = y + scy;
            uint8_t bg_x = x + scx;

            uint16_t tile_map_addr = map_base + ((bg_y / 8) * 32) + (bg_x / 8);
            uint16_t map_offset = tile_map_addr - 0x8000;

            int tile_id;
            uint8_t attr = 0;
            uint8_t tile_vram_bank = 0;
            uint8_t cgb_pal = 0;
            int x_flip = 0, y_flip = 0;

            if (cpu->cgb_mode) {
                tile_id = cpu->vram_banks[0][map_offset];
                attr = cpu->vram_banks[1][map_offset];
                cgb_pal = attr & 0x07;
                tile_vram_bank = (attr >> 3) & 1;
                x_flip = attr & 0x20;
                y_flip = attr & 0x40;
            } else {
                tile_id = cpu->memory[tile_map_addr];
            }
            if (is_signed) tile_id = (int8_t)tile_id;

            uint16_t tile_data_addr = is_signed ? (data_base + (tile_id + 128) * 16) : (data_base + tile_id * 16);
            uint16_t data_offset = tile_data_addr - 0x8000;

            int row = bg_y % 8;
            if (y_flip) row = 7 - row;

            uint8_t byte1, byte2;
            if (cpu->cgb_mode) {
                byte1 = cpu->vram_banks[tile_vram_bank][data_offset + row * 2];
                byte2 = cpu->vram_banks[tile_vram_bank][data_offset + row * 2 + 1];
            } else {
                byte1 = cpu->memory[tile_data_addr + row * 2];
                byte2 = cpu->memory[tile_data_addr + row * 2 + 1];
            }

            int col = bg_x % 8;
            int bit = x_flip ? col : (7 - col);
            uint8_t color_id = ((byte2 >> bit) & 0x1) << 1 | ((byte1 >> bit) & 0x1);

            if (cpu->cgb_mode) {
                pixels[y * 160 + x] = get_cgb_bg_color(cpu, cgb_pal, color_id);
            } else {
                uint8_t bgp = cpu->memory[0xFF47];
                uint8_t actual_color = (bgp >> (color_id * 2)) & 0x03;
                pixels[y * 160 + x] = palette[actual_color];
            }
        }
    }
}

void render_window(cpu_t *cpu, uint32_t *pixels) {
    uint8_t lcdc = cpu->memory[0xFF40];

    if (!(lcdc & 0x20) || !(lcdc & 0x80)) return;

    uint8_t wx = cpu->memory[0xFF4B];
    uint8_t wy = cpu->memory[0xFF4A];

    if (wx > 166 || wy > 143) return;

    int wx_corrected = wx - 7;

    uint16_t map_base = (lcdc & 0x40) ? 0x9C00 : 0x9800;
    uint16_t data_base = (lcdc & 0x10) ? 0x8000 : 0x8800;
    int is_signed = !(lcdc & 0x10);

    for (int y = 0; y < 144; y++) {
        if (y < wy) continue;

        for (int x = 0; x < 160; x++) {
            if (x < wx_corrected) continue;

            uint8_t w_y = y - wy;
            uint8_t w_x = x - wx_corrected;

            uint16_t tile_map_addr = map_base + ((w_y / 8) * 32) + (w_x / 8);
            uint16_t map_offset = tile_map_addr - 0x8000;

            int tile_id;
            uint8_t attr = 0;
            uint8_t tile_vram_bank = 0;
            uint8_t cgb_pal = 0;
            int x_flip = 0, y_flip = 0;

            if (cpu->cgb_mode) {
                tile_id = cpu->vram_banks[0][map_offset];
                attr = cpu->vram_banks[1][map_offset];
                cgb_pal = attr & 0x07;
                tile_vram_bank = (attr >> 3) & 1;
                x_flip = attr & 0x20;
                y_flip = attr & 0x40;
            } else {
                tile_id = cpu->memory[tile_map_addr];
            }
            if (is_signed) tile_id = (int8_t)tile_id;

            uint16_t tile_data_addr = is_signed ? (data_base + (tile_id + 128) * 16) : (data_base + tile_id * 16);
            uint16_t data_offset = tile_data_addr - 0x8000;

            int row = w_y % 8;
            if (y_flip) row = 7 - row;

            uint8_t byte1, byte2;
            if (cpu->cgb_mode) {
                byte1 = cpu->vram_banks[tile_vram_bank][data_offset + row * 2];
                byte2 = cpu->vram_banks[tile_vram_bank][data_offset + row * 2 + 1];
            } else {
                byte1 = cpu->memory[tile_data_addr + row * 2];
                byte2 = cpu->memory[tile_data_addr + row * 2 + 1];
            }

            int col = w_x % 8;
            int bit = x_flip ? col : (7 - col);
            uint8_t color_id = ((byte2 >> bit) & 0x1) << 1 | ((byte1 >> bit) & 0x1);

            if (cpu->cgb_mode) {
                pixels[y * 160 + x] = get_cgb_bg_color(cpu, cgb_pal, color_id);
            } else {
                uint8_t bgp = cpu->memory[0xFF47];
                uint8_t actual_color = (bgp >> (color_id * 2)) & 0x03;
                pixels[y * 160 + x] = palette[actual_color];
            }
        }
    }
}

void render_sprites(cpu_t *cpu, uint32_t *pixels) {
    if (!(cpu->memory[0xFF40] & 0x02)) return;

    int use_8x16 = (cpu->memory[0xFF40] & 0x04);

    for (int i = 0; i < 40; i++) {
        uint16_t oam_addr = 0xFE00 + (i * 4);
        int y = (int)cpu->memory[oam_addr] - 16;
        int x = (int)cpu->memory[oam_addr + 1] - 8;
        uint8_t tile_id = cpu->memory[oam_addr + 2];
        uint8_t attributes = cpu->memory[oam_addr + 3];

        if (use_8x16) tile_id &= 0xFE;

        int height = use_8x16 ? 16 : 8;

        if (y <= -height || y >= 144 || x <= -8 || x >= 160) continue;

        uint16_t tile_data_addr = 0x8000 + (tile_id * 16);
        uint16_t data_offset = tile_data_addr - 0x8000;
        int x_flip = (attributes & 0x20);
        int y_flip = (attributes & 0x40);

        // CGB: bit 0-2 = palette, bit 3 = vram bank
        uint8_t cgb_pal = attributes & 0x07;
        uint8_t tile_vram_bank = (attributes >> 3) & 1;
        // DMG: bit 4 = palette select
        uint8_t obp = (attributes & 0x10) ? cpu->memory[0xFF49] : cpu->memory[0xFF48];

        for (int ty = 0; ty < height; ty++) {
            int draw_y = y + ty;
            if (draw_y < 0 || draw_y >= 144) continue;

            int line_in_tile = y_flip ? (height - 1 - ty) : ty;

            uint16_t current_offset = data_offset;
            if (line_in_tile >= 8) {
                current_offset += 16;
                line_in_tile -= 8;
            }

            uint8_t byte1, byte2;
            if (cpu->cgb_mode) {
                byte1 = cpu->vram_banks[tile_vram_bank][current_offset + line_in_tile * 2];
                byte2 = cpu->vram_banks[tile_vram_bank][current_offset + line_in_tile * 2 + 1];
            } else {
                uint16_t current_tile_addr = tile_data_addr;
                if (ty >= 8 || (y_flip && ty < height - 8)) current_tile_addr += (current_offset - data_offset);
                byte1 = cpu->memory[current_offset + 0x8000 + line_in_tile * 2];
                byte2 = cpu->memory[current_offset + 0x8000 + line_in_tile * 2 + 1];
            }

            for (int tx = 0; tx < 8; tx++) {
                int draw_x = x + tx;
                if (draw_x < 0 || draw_x >= 160) continue;

                int bit = x_flip ? tx : (7 - tx);
                uint8_t color_id = ((byte2 >> bit) & 0x1) << 1 | ((byte1 >> bit) & 0x1);
                if (color_id == 0) continue;

                if (cpu->cgb_mode) {
                    pixels[draw_y * 160 + draw_x] = get_cgb_obj_color(cpu, cgb_pal, color_id);
                } else {
                    uint8_t actual_color = (obp >> (color_id * 2)) & 0x03;
                    pixels[draw_y * 160 + draw_x] = palette[actual_color];
                }
            }
        }
    }
}

void update_graphics(cpu_t *cpu, int cycles)
{
    static int cycle_count = 0;
    cycle_count += cycles;

    if (!(read_8(cpu, 0xFF40) & 0x80)) {
        while (cycle_count >= 456) {
            cycle_count -= 456;
            uint8_t current_ly = read_8(cpu, 0xFF44);
            write_8(cpu, 0xFF44, current_ly + 1);
            if (read_8(cpu, 0xFF44) > 153) write_8(cpu, 0xFF44, 0);
        }
        write_8(cpu, 0xFF41, (read_8(cpu, 0xFF41) & ~0x03) | 0x00);
        return;
    }

    uint8_t ly = cpu->memory[0xFF44];
    uint8_t stat = cpu->memory[0xFF41];
    uint8_t old_mode = stat & 0x03;
    uint8_t new_mode = old_mode;

    // Update Mode
    if (ly >= 144) {
        new_mode = 1; // Mode 1
    } else {
        if (cycle_count <= 80) new_mode = 2; // Mode 2
        else if (cycle_count <= 252) new_mode = 3; // Mode 3
        else new_mode = 0; // Mode 0
    }

    // STAT Interrupt on mode change
    if (new_mode != old_mode) {
        if (new_mode == 0 && (stat & 0x08)) write_8(cpu, 0xFF0F, read_8(cpu, 0xFF0F) | 0x02);
        if (new_mode == 1 && (stat & 0x10)) write_8(cpu, 0xFF0F, read_8(cpu, 0xFF0F) | 0x02);
        if (new_mode == 2 && (stat & 0x20)) write_8(cpu, 0xFF0F, read_8(cpu, 0xFF0F) | 0x02);
    }

    stat = (stat & ~0x03) | new_mode;

    // LYC == LY comparison
    if (ly == read_8(cpu, 0xFF45)) {
        if (!(stat & 0x04)) {
            stat |= 0x04;
            if (stat & 0x40) write_8(cpu, 0xFF0F, read_8(cpu, 0xFF0F) | 0x02); // STAT interrupt
        }
    } else {
        stat &= ~0x04;
    }

    write_8(cpu, 0xFF41, stat);

    while (cycle_count >= 456) {
        cycle_count -= 456;
        uint8_t current_ly = read_8(cpu, 0xFF44);
        write_8(cpu, 0xFF44, current_ly + 1);
        if (read_8(cpu, 0xFF44) > 153) write_8(cpu, 0xFF44, 0);

        if (read_8(cpu, 0xFF44) == 144)
            write_8(cpu, 0xFF0F, read_8(cpu, 0xFF0F) | 0x01);
    }
}