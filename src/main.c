#include <SDL2/SDL.h>
#include <stdio.h>
#include "cpu.h"

static uint8_t turbo_mode = 0;

static void init_cpu(cpu_t *cpu)
{
    cpu->pc = 0x0100;
    cpu->sp = 0xFFFE;
    cpu->registers.f = 0x80;
    cpu->registers.b = 0x00;
    cpu->registers.c = 0x13;
    cpu->registers.d = 0x00;
    cpu->registers.e = 0xD8;
    cpu->registers.h = 0x01;
    cpu->registers.l = 0x4D;

    if (cpu->cgb_mode) {
        cpu->registers.a = 0x11; // CGB boot leaves A=0x11
    } else {
        cpu->registers.a = 0x01;
    }

    write_8(cpu, 0xFF04, 0x00);
    write_8(cpu, 0xFF05, 0x00);
    write_8(cpu, 0xFF06, 0x00);
    write_8(cpu, 0xFF07, 0x00);
    write_8(cpu, 0xFF40, 0x91);
    write_8(cpu, 0xFF47, 0xFC);
}

static void get_rom_title(cpu_t *cpu, char *buf, int len)
{
    int i = 0;
    for (uint16_t a = 0x0134; a <= 0x0143 && i < len - 1; a++) {
        uint8_t c = cpu->rom[a];
        if (c == 0) break;
        buf[i++] = c;
    }
    buf[i] = '\0';
}

void set_turbo(uint8_t on) { turbo_mode = on; }
uint8_t get_turbo(void) { return turbo_mode; }

int main(int argc, char **argv)
{
    cpu_t cpu = {0};
    char *path = (argc > 1) ? argv[1] : "./assets/test.gb";

    read_rom(path, &cpu);
    init_cpu(&cpu);
    init_display();
    init_apu();
    init_save(path, &cpu);

    char rom_title[17];
    get_rom_title(&cpu, rom_title, sizeof(rom_title));
    printf("Loaded: %s\n", rom_title);

    uint8_t last_ly = 0;
    int frame_count = 0;
    uint32_t fps_timer = SDL_GetTicks();

    double target_frame_time = 1000.0 / 59.73;
    uint64_t perf_freq = SDL_GetPerformanceFrequency();
    uint64_t frame_start = SDL_GetPerformanceCounter();

    for (;;) {
        if (cpu.ime_scheduled > 0) {
            if (cpu.ime_scheduled == 1) cpu.ime = 1;
            cpu.ime_scheduled--;
        }

        int c = 4;

        if (cpu.halted) {
            if (read_8(&cpu, 0xFF0F) & read_8(&cpu, 0xFFFF)) {
                cpu.halted = 0;
                cpu.pc++;
            }
        } else {
            c = execute_instruction(&cpu);
        }

        update_timers(&cpu, c);
        update_graphics(&cpu, c);
        update_audio(c);
        handle_interrupts(&cpu);

        uint8_t ly = read_8(&cpu, 0xFF44);
        if (ly == 144 && last_ly != 144) {
            update_display(&cpu);
            update_input(&cpu);
            frame_count++;

            // Update title every second
            uint32_t now = SDL_GetTicks();
            if (now - fps_timer >= 1000) {
                char title[128];
                snprintf(title, sizeof(title), "%s | %d FPS%s",
                    rom_title, frame_count, turbo_mode ? " | TURBO" : "");
                SDL_SetWindowTitle(SDL_GetWindowFromID(1), title);
                frame_count = 0;
                fps_timer = now;
            }

            if (!turbo_mode) {
                uint64_t frame_end = SDL_GetPerformanceCounter();
                double elapsed = (double)(frame_end - frame_start) * 1000.0 / perf_freq;
                if (elapsed < target_frame_time)
                    SDL_Delay((uint32_t)(target_frame_time - elapsed));
            }
            frame_start = SDL_GetPerformanceCounter();
        }
        last_ly = ly;
    }
}