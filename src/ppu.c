#include <SDL2/SDL.h>
#include <string.h>
#include "cpu.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static uint32_t screen_pixels[160 * 144];

extern void set_turbo(uint8_t on);
extern uint8_t get_turbo(void);

void init_display(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0)
        return;
    window = SDL_CreateWindow("GameBoy Emulator",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        160 * 4, 144 * 4, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    memset(screen_pixels, 0, sizeof(screen_pixels));
}

void update_input(cpu_t *cpu)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            write_save(cpu);
            cleanup_apu();
            exit(0);
        }
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            uint8_t bit = 0xFF;
            switch (e.key.keysym.sym) {
                case SDLK_RIGHT:  bit = 0; break;
                case SDLK_LEFT:   bit = 1; break;
                case SDLK_UP:     bit = 2; break;
                case SDLK_DOWN:   bit = 3; break;
                case SDLK_a:      bit = 4; break;
                case SDLK_s:      bit = 5; break;
                case SDLK_SPACE:  bit = 6; break;
                case SDLK_RETURN: bit = 7; break;
                case SDLK_TAB:
                    set_turbo(e.type == SDL_KEYDOWN);
                    break;
                case SDLK_F5:
                    if (e.type == SDL_KEYDOWN) {
                        save_state(cpu);
                        write_save(cpu);
                    }
                    break;
                case SDLK_F8:
                    if (e.type == SDL_KEYDOWN)
                        load_state(cpu);
                    break;
                default: break;
            }
            if (bit != 0xFF) {
                if (e.type == SDL_KEYDOWN) {
                    cpu->joypad_state |= (1 << bit);
                    write_8(cpu, 0xFF0F, read_8(cpu, 0xFF0F) | 0x10);
                } else {
                    cpu->joypad_state &= ~(1 << bit);
                }
            }
        }
    }
}

void handle_interrupts(cpu_t *cpu)
{
    if (!cpu->ime || cpu->halted)
        return;

    uint8_t pending = read_8(cpu, 0xFF0F) & read_8(cpu, 0xFFFF);
    uint8_t vectors[] = {0x40, 0x48, 0x50, 0x58, 0x60};

    for (int i = 0; i < 5; i++) {
        if (pending & (1 << i)) {
            cpu->ime = 0;
            write_8(cpu, 0xFF0F, read_8(cpu, 0xFF0F) & ~(1 << i));
            stack_push16(cpu, cpu->pc);
            cpu->pc = vectors[i];
            return;
        }
    }
}

void update_display(cpu_t *cpu)
{
    if (!(cpu->memory[0xFF40] & 0x80))
        return;
    render_background(cpu, screen_pixels);
    render_window(cpu, screen_pixels);
    render_sprites(cpu, screen_pixels);
    SDL_UpdateTexture(texture, NULL, screen_pixels, 160 * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}