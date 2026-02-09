#include <SDL2/SDL.h>
#include <string.h>
#include "cpu.h"

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 1024

static SDL_AudioDeviceID audio_dev;

static const uint8_t duty_table[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0},
};

typedef struct {
    uint8_t enabled;
    uint8_t duty;
    uint8_t duty_pos;
    uint16_t freq;
    float timer;
    uint8_t volume;
    uint8_t volume_init;
    uint8_t envelope_dir;
    uint8_t envelope_period;
    float envelope_timer;
    uint8_t length;
    uint8_t length_enable;
    float length_timer;
    // Channel 1 sweep
    uint8_t sweep_period;
    uint8_t sweep_dir;
    uint8_t sweep_shift;
    float sweep_timer;
    uint16_t sweep_freq;
} channel_t;

typedef struct {
    channel_t ch1;
    channel_t ch2;
    uint8_t master_enable;
    uint8_t left_volume;
    uint8_t right_volume;
    uint8_t ch_select;
} apu_t;

static apu_t apu;

void init_apu(void)
{
    memset(&apu, 0, sizeof(apu));
    apu.master_enable = 1;
    apu.left_volume = 7;
    apu.right_volume = 7;

    SDL_AudioSpec want = {0}, have;
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = BUFFER_SIZE;
    want.callback = NULL;

    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_dev)
        SDL_PauseAudioDevice(audio_dev, 0);
}

static void tick_channel(channel_t *ch, float dt)
{
    if (!ch->enabled)
        return;

    // Length counter
    if (ch->length_enable) {
        ch->length_timer += dt;
        float len_sec = (64 - ch->length) / 256.0f;
        if (ch->length_timer >= len_sec) {
            ch->enabled = 0;
            return;
        }
    }

    // Envelope
    if (ch->envelope_period > 0) {
        ch->envelope_timer += dt;
        float env_step = ch->envelope_period / 64.0f;
        while (ch->envelope_timer >= env_step) {
            ch->envelope_timer -= env_step;
            if (ch->envelope_dir && ch->volume < 15)
                ch->volume++;
            else if (!ch->envelope_dir && ch->volume > 0)
                ch->volume--;
        }
    }

    // Duty cycle stepping
    float period = (2048 - ch->freq) * 4.0f / 4194304.0f;
    if (period > 0) {
        ch->timer += dt;
        while (ch->timer >= period) {
            ch->timer -= period;
            ch->duty_pos = (ch->duty_pos + 1) & 7;
        }
    }
}

static void tick_sweep(channel_t *ch, float dt)
{
    if (ch->sweep_period == 0)
        return;
    ch->sweep_timer += dt;
    float sweep_step = ch->sweep_period / 128.0f;
    while (ch->sweep_timer >= sweep_step && ch->enabled) {
        ch->sweep_timer -= sweep_step;
        uint16_t delta = ch->sweep_freq >> ch->sweep_shift;
        uint16_t new_freq;
        if (ch->sweep_dir)
            new_freq = ch->sweep_freq - delta;
        else
            new_freq = ch->sweep_freq + delta;
        if (new_freq > 2047) {
            ch->enabled = 0;
        } else if (ch->sweep_shift > 0) {
            ch->sweep_freq = new_freq;
            ch->freq = new_freq;
        }
    }
}

static int8_t sample_channel(channel_t *ch)
{
    if (!ch->enabled || ch->volume == 0)
        return 0;
    int8_t out = duty_table[ch->duty][ch->duty_pos] ? ch->volume : -ch->volume;
    return out;
}

void apu_write(cpu_t *cpu, uint16_t addr, uint8_t val)
{
    (void)cpu;
    switch (addr) {
    // Channel 1 - Sweep
    case 0xFF10:
        apu.ch1.sweep_period = (val >> 4) & 7;
        apu.ch1.sweep_dir = (val >> 3) & 1;
        apu.ch1.sweep_shift = val & 7;
        break;
    case 0xFF11:
        apu.ch1.duty = (val >> 6) & 3;
        apu.ch1.length = val & 0x3F;
        break;
    case 0xFF12:
        apu.ch1.volume_init = (val >> 4) & 0xF;
        apu.ch1.envelope_dir = (val >> 3) & 1;
        apu.ch1.envelope_period = val & 7;
        break;
    case 0xFF13:
        apu.ch1.freq = (apu.ch1.freq & 0x700) | val;
        break;
    case 0xFF14:
        apu.ch1.freq = (apu.ch1.freq & 0xFF) | ((val & 7) << 8);
        apu.ch1.length_enable = (val >> 6) & 1;
        if (val & 0x80) {
            apu.ch1.enabled = 1;
            apu.ch1.volume = apu.ch1.volume_init;
            apu.ch1.envelope_timer = 0;
            apu.ch1.length_timer = 0;
            apu.ch1.sweep_freq = apu.ch1.freq;
            apu.ch1.sweep_timer = 0;
        }
        break;
    // Channel 2
    case 0xFF16:
        apu.ch2.duty = (val >> 6) & 3;
        apu.ch2.length = val & 0x3F;
        break;
    case 0xFF17:
        apu.ch2.volume_init = (val >> 4) & 0xF;
        apu.ch2.envelope_dir = (val >> 3) & 1;
        apu.ch2.envelope_period = val & 7;
        break;
    case 0xFF18:
        apu.ch2.freq = (apu.ch2.freq & 0x700) | val;
        break;
    case 0xFF19:
        apu.ch2.freq = (apu.ch2.freq & 0xFF) | ((val & 7) << 8);
        apu.ch2.length_enable = (val >> 6) & 1;
        if (val & 0x80) {
            apu.ch2.enabled = 1;
            apu.ch2.volume = apu.ch2.volume_init;
            apu.ch2.envelope_timer = 0;
            apu.ch2.length_timer = 0;
        }
        break;
    // Master control
    case 0xFF24:
        apu.left_volume = (val >> 4) & 7;
        apu.right_volume = val & 7;
        break;
    case 0xFF25:
        apu.ch_select = val;
        break;
    case 0xFF26:
        apu.master_enable = (val >> 7) & 1;
        break;
    }
}

void update_audio(int cycles)
{
    if (!audio_dev || !apu.master_enable)
        return;

    float dt = cycles / 4194304.0f;

    tick_channel(&apu.ch1, dt);
    tick_sweep(&apu.ch1, dt);
    tick_channel(&apu.ch2, dt);

    static float sample_timer = 0;
    static int16_t sample_buf[BUFFER_SIZE * 2];
    static int sample_pos = 0;

    sample_timer += dt;
    float sample_period = 1.0f / SAMPLE_RATE;

    while (sample_timer >= sample_period) {
        sample_timer -= sample_period;

        int8_t s1 = sample_channel(&apu.ch1);
        int8_t s2 = sample_channel(&apu.ch2);

        int16_t left = 0, right = 0;
        if (apu.ch_select & 0x10) left += s1;
        if (apu.ch_select & 0x01) right += s1;
        if (apu.ch_select & 0x20) left += s2;
        if (apu.ch_select & 0x02) right += s2;

        left = left * (apu.left_volume + 1) * 64;
        right = right * (apu.right_volume + 1) * 64;

        sample_buf[sample_pos++] = left;
        sample_buf[sample_pos++] = right;

        if (sample_pos >= BUFFER_SIZE * 2) {
            // Don't queue if too much buffered
            if (SDL_GetQueuedAudioSize(audio_dev) < BUFFER_SIZE * 4 * 4)
                SDL_QueueAudio(audio_dev, sample_buf, sample_pos * sizeof(int16_t));
            sample_pos = 0;
        }
    }
}

void cleanup_apu(void)
{
    if (audio_dev) {
        SDL_CloseAudioDevice(audio_dev);
        audio_dev = 0;
    }
}
