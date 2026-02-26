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
    uint8_t sweep_period;
    uint8_t sweep_dir;
    uint8_t sweep_shift;
    float sweep_timer;
    uint16_t sweep_freq;
} channel_t;

typedef struct {
    uint8_t enabled;
    uint16_t freq;
    float timer;
    uint8_t volume_shift;
    uint8_t sample_pos;
    uint8_t wave_ram[16]; // 32 4-bit samples packed into 16 bytes
    uint16_t length;
    uint8_t length_enable;
    float length_timer;
    uint8_t dac_enable;
} wave_channel_t;

typedef struct {
    uint8_t enabled;
    float timer;
    uint8_t volume;
    uint8_t volume_init;
    uint8_t envelope_dir;
    uint8_t envelope_period;
    float envelope_timer;
    uint8_t length;
    uint8_t length_enable;
    float length_timer;
    uint8_t clock_shift;
    uint8_t width_mode; // 0=15-bit, 1=7-bit
    uint8_t divisor_code;
    uint16_t lfsr;
} noise_channel_t;

typedef struct {
    channel_t ch1;
    channel_t ch2;
    wave_channel_t ch3;
    noise_channel_t ch4;
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
    apu.ch4.lfsr = 0x7FFF;

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
    if (ch->length_enable) {
        ch->length_timer += dt;
        if (ch->length_timer >= (64 - ch->length) / 256.0f) {
            ch->enabled = 0;
            return;
        }
    }
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
        uint16_t new_freq = ch->sweep_dir ? ch->sweep_freq - delta : ch->sweep_freq + delta;
        if (new_freq > 2047) {
            ch->enabled = 0;
        } else if (ch->sweep_shift > 0) {
            ch->sweep_freq = new_freq;
            ch->freq = new_freq;
        }
    }
}

static void tick_wave(wave_channel_t *ch, float dt)
{
    if (!ch->enabled || !ch->dac_enable)
        return;
    if (ch->length_enable) {
        ch->length_timer += dt;
        if (ch->length_timer >= (256 - ch->length) / 256.0f) {
            ch->enabled = 0;
            return;
        }
    }
    float period = (2048 - ch->freq) * 2.0f / 4194304.0f;
    if (period > 0) {
        ch->timer += dt;
        while (ch->timer >= period) {
            ch->timer -= period;
            ch->sample_pos = (ch->sample_pos + 1) & 31;
        }
    }
}

static void tick_noise(noise_channel_t *ch, float dt)
{
    if (!ch->enabled)
        return;
    if (ch->length_enable) {
        ch->length_timer += dt;
        if (ch->length_timer >= (64 - ch->length) / 256.0f) {
            ch->enabled = 0;
            return;
        }
    }
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
    static const uint8_t divisors[] = {8, 16, 32, 48, 64, 80, 96, 112};
    float period = (divisors[ch->divisor_code] << ch->clock_shift) / 4194304.0f;
    if (period > 0) {
        ch->timer += dt;
        while (ch->timer >= period) {
            ch->timer -= period;
            uint8_t xor_bit = (ch->lfsr & 1) ^ ((ch->lfsr >> 1) & 1);
            ch->lfsr = (ch->lfsr >> 1) | (xor_bit << 14);
            if (ch->width_mode)
                ch->lfsr = (ch->lfsr & ~(1 << 6)) | (xor_bit << 6);
        }
    }
}

static int8_t sample_channel(channel_t *ch)
{
    if (!ch->enabled || ch->volume == 0)
        return 0;
    return duty_table[ch->duty][ch->duty_pos] ? ch->volume : -ch->volume;
}

static int8_t sample_wave(wave_channel_t *ch)
{
    if (!ch->enabled || !ch->dac_enable || ch->volume_shift == 0)
        return 0;
    uint8_t byte = ch->wave_ram[ch->sample_pos / 2];
    uint8_t sample = (ch->sample_pos & 1) ? (byte & 0x0F) : (byte >> 4);
    sample >>= (ch->volume_shift - 1);
    return (int8_t)sample - 8;
}

static int8_t sample_noise(noise_channel_t *ch)
{
    if (!ch->enabled || ch->volume == 0)
        return 0;
    return (ch->lfsr & 1) ? -ch->volume : ch->volume;
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
    // Channel 3 - Wave
    case 0xFF1A:
        apu.ch3.dac_enable = (val >> 7) & 1;
        if (!apu.ch3.dac_enable) apu.ch3.enabled = 0;
        break;
    case 0xFF1B:
        apu.ch3.length = val;
        break;
    case 0xFF1C:
        apu.ch3.volume_shift = (val >> 5) & 3; // 0=mute, 1=100%, 2=50%, 3=25%
        break;
    case 0xFF1D:
        apu.ch3.freq = (apu.ch3.freq & 0x700) | val;
        break;
    case 0xFF1E:
        apu.ch3.freq = (apu.ch3.freq & 0xFF) | ((val & 7) << 8);
        apu.ch3.length_enable = (val >> 6) & 1;
        if (val & 0x80) {
            apu.ch3.enabled = apu.ch3.dac_enable;
            apu.ch3.length_timer = 0;
            apu.ch3.sample_pos = 0;
            apu.ch3.timer = 0;
        }
        break;
    // Channel 4 - Noise
    case 0xFF20:
        apu.ch4.length = val & 0x3F;
        break;
    case 0xFF21:
        apu.ch4.volume_init = (val >> 4) & 0xF;
        apu.ch4.envelope_dir = (val >> 3) & 1;
        apu.ch4.envelope_period = val & 7;
        break;
    case 0xFF22:
        apu.ch4.clock_shift = (val >> 4) & 0xF;
        apu.ch4.width_mode = (val >> 3) & 1;
        apu.ch4.divisor_code = val & 7;
        break;
    case 0xFF23:
        apu.ch4.length_enable = (val >> 6) & 1;
        if (val & 0x80) {
            apu.ch4.enabled = 1;
            apu.ch4.volume = apu.ch4.volume_init;
            apu.ch4.envelope_timer = 0;
            apu.ch4.length_timer = 0;
            apu.ch4.lfsr = 0x7FFF;
            apu.ch4.timer = 0;
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
        if (!apu.master_enable) {
            apu.ch1.enabled = 0;
            apu.ch2.enabled = 0;
            apu.ch3.enabled = 0;
            apu.ch4.enabled = 0;
        }
        break;
    default:
        // Wave RAM (0xFF30-0xFF3F)
        if (addr >= 0xFF30 && addr <= 0xFF3F)
            apu.ch3.wave_ram[addr - 0xFF30] = val;
        break;
    }
}

uint8_t apu_read(uint16_t addr)
{
    switch (addr) {
    case 0xFF26: {
        uint8_t status = (apu.master_enable << 7) | 0x70;
        if (apu.ch1.enabled) status |= 0x01;
        if (apu.ch2.enabled) status |= 0x02;
        if (apu.ch3.enabled) status |= 0x04;
        if (apu.ch4.enabled) status |= 0x08;
        return status;
    }
    case 0xFF25: return apu.ch_select;
    case 0xFF24: return (apu.left_volume << 4) | apu.right_volume;
    default:
        if (addr >= 0xFF30 && addr <= 0xFF3F)
            return apu.ch3.wave_ram[addr - 0xFF30];
        return 0xFF;
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
    tick_wave(&apu.ch3, dt);
    tick_noise(&apu.ch4, dt);

    static float sample_timer = 0;
    static int16_t sample_buf[BUFFER_SIZE * 2];
    static int sample_pos = 0;

    sample_timer += dt;
    float sample_period = 1.0f / SAMPLE_RATE;

    while (sample_timer >= sample_period) {
        sample_timer -= sample_period;

        int8_t s1 = sample_channel(&apu.ch1);
        int8_t s2 = sample_channel(&apu.ch2);
        int8_t s3 = sample_wave(&apu.ch3);
        int8_t s4 = sample_noise(&apu.ch4);

        int16_t left = 0, right = 0;
        if (apu.ch_select & 0x10) left += s1;
        if (apu.ch_select & 0x01) right += s1;
        if (apu.ch_select & 0x20) left += s2;
        if (apu.ch_select & 0x02) right += s2;
        if (apu.ch_select & 0x40) left += s3;
        if (apu.ch_select & 0x04) right += s3;
        if (apu.ch_select & 0x80) left += s4;
        if (apu.ch_select & 0x08) right += s4;

        left = left * (apu.left_volume + 1) * 48;
        right = right * (apu.right_volume + 1) * 48;

        sample_buf[sample_pos++] = left;
        sample_buf[sample_pos++] = right;

        if (sample_pos >= BUFFER_SIZE * 2) {
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
