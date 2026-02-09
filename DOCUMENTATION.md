# Game Boy Emulator Exhaustive Documentation

## 1. CPU Architecture (Sharp SM83)
The CPU is an 8-bit hybrid between the Intel 8080 and the Zilog Z80.

### 1.1 Registers
- **8-bit General Purpose:**
  - `A` (Accumulator): Used for arithmetic and logic results.
  - `F` (Flags): Stores the state of the last operation.
  - `B`, `C`, `D`, `E`, `H`, `L`: General storage.
- **16-bit Virtual Pairs:**
  - `AF`, `BC`, `DE`, `HL`.
- **Special Registers:**
  - `PC` (Program Counter): 16-bit address of the current instruction.
  - `SP` (Stack Pointer): 16-bit address of the top of the stack.

### 1.2 The Flags Register (F)
| Bit | Name | Description |
|-----|------|-------------|
| 7   | Z    | **Zero Flag**: Set if result was zero. |
| 6   | N    | **Subtract Flag**: Set if last op was subtraction (BCD). |
| 5   | H    | **Half Carry**: Set if carry from bit 3 to 4. |
| 4   | C    | **Carry Flag**: Set if carry from bit 7. |

---

## 2. Memory Map
The Game Boy uses a 16-bit address space (0x0000 to 0xFFFF).

| Range | Usage | Description |
|-------|-------|-------------|
| 0x0000 - 0x3FFF | ROM Bank 0 | Fixed first half of the game code. |
| 0x4000 - 0x7FFF | ROM Bank 1-N | Switchable banks via MBC. |
| 0x8000 - 0x9FFF | VRAM | Stores tile patterns and tile maps. |
| 0xFE00 - 0xFE9F | OAM | Object Attribute Memory (Sprite data). |
| 0xFF00 - 0xFF7F | I/O Ports | Hardware registers (Graphics, Sound, Input). |
| 0xFF80 - 0xFFFE | HRAM | High RAM (Faster access). |
| 0xFFFF          | IE    | Interrupt Enable Register. |

---

## 3. Graphics (PPU)
The PPU draws 160x144 pixels to the screen.

### 3.1 LCD Control Register (0xFF40 - LCDC)
- **Bit 7:** LCD Enable.
- **Bit 4:** BG/Window Tile Data Select (0=0x8800, 1=0x8000).
- **Bit 1:** Sprite Enable.
- **Bit 0:** BG/Window Display Enable.

### 3.2 Pixel Pipeline
1. **Fetch Tile ID:** Look up index in Tile Map (0x9800 or 0x9C00).
2. **Fetch Tile Data:** Read 16 bytes for that ID from Tile Data.
3. **Apply Palette:** Map 2-bit color ID to one of 4 colors via `BGP` register.
4. **Display:** Push pixels to the SDL texture.

---

## 4. Input System (0xFF00 - JOYP)
Input uses a matrix where the CPU "selects" a row by writing to bits 4 or 5.
- **Bit 5=0:** Select Buttons (A, B, Select, Start).
- **Bit 4=0:** Select Directions (Right, Left, Up, Down).
- **Bits 0-3:** Return the state (0 = Pressed).

---

## 5. Timers and Randomness
- **DIV (0xFF04):** The Divider Register. It increments at a fixed rate of 16384 Hz (every 256 clock cycles). Writing any value to this register resets it to 0. It is often used by games as a source of entropy for random numbers.
- **TIMA (0xFF05):** The Timer Counter. This register is incremented at a frequency specified by the `TAC` register. When it overflows (exceeds 0xFF), it is reset to the value in `TMA`, and a Timer Interrupt is requested.
- **TMA (0xFF06):** The Timer Modulo. When `TIMA` overflows, it is reloaded with this value.
- **TAC (0xFF07):** Timer Control. 
  - **Bit 2:** Timer Enable (1=Enabled, 0=Disabled).
  - **Bits 0-1:** Input Clock Select (00: 4096 Hz, 01: 262144 Hz, 10: 65536 Hz, 11: 16384 Hz).

---

## 6. Interrupts
The Game Boy has 5 interrupt sources. When an interrupt is triggered and the Interrupt Master Enable (IME) flag is set, the CPU pushes the current PC onto the stack and jumps to a specific handler address.

| Interrupt | Source | Handler Address | Bit in IF/IE |
|-----------|--------|-----------------|--------------|
| VBlank    | LCD    | 0x0040          | Bit 0        |
| LCD STAT  | LCD    | 0x0048          | Bit 1        |
| Timer     | Timers | 0x0050          | Bit 2        |
| Serial    | Link   | 0x0058          | Bit 3        |
| Joypad    | Input  | 0x0060          | Bit 4        |

---

## 7. Special Instructions
### 7.1 HALT (0x76)
The `HALT` instruction puts the CPU into a low-power state. It stops executing instructions until an interrupt occurs. If interrupts are disabled (`IME=0`) but an interrupt is pending, the CPU may experience a "HALT bug" (repeating the next instruction), but in most implementations, it simply resumes execution when an interrupt flag is set in the `IF` register.

### 7.2 STOP (0x10)
Used to enter a very low-power standby mode. In the original Game Boy, it stops the CPU and LCD. On the Game Boy Color, it is also used to switch CPU speeds.

---

## 7. Cartridge Banking (MBC)
Since the Game Boy can only address 32KB of ROM at once, larger games (like Pokémon) use a **Memory Bank Controller (MBC)** chip inside the cartridge.

### 7.1 MBC1 (Common)
- **ROM Bank 0 (0x0000-0x3FFF):** Always stays at the start of the ROM.
- **ROM Bank 1-N (0x4000-0x7FFF):** Swapped out by writing a bank number to the address range `0x2000-0x3FFF`.
- **RAM Banking:** Some cartridges have extra Save RAM (SRAM) at `0xA000-0xBFFF`.

### 7.2 Cartridge Header (0x0147)
This byte tells you what hardware is in the cartridge:
- `0x00`: ROM ONLY (Tetris)
- `0x01`: MBC1
- `0x03`: MBC1 + RAM + BATTERY (Pokémon - allows saving)
- `0x13`: MBC3 + RAM + BATTERY (Pokémon Gold/Silver)
- `0x1B`: MBC5 + RAM + BATTERY (Pokémon Red/Blue international)

---

## 8. Sound (APU)
The Game Boy has 4 sound channels:
- **Channel 1:** Pulse wave with sweep and envelope (NR10-NR14: 0xFF10-0xFF14)
- **Channel 2:** Pulse wave with envelope (NR21-NR24: 0xFF16-0xFF19)
- **Channel 3:** Wave output (NR30-NR34: 0xFF1A-0xFF1E, Wave RAM: 0xFF30-0xFF3F)
- **Channel 4:** Noise (NR41-NR44: 0xFF20-0xFF23)

Master control: NR50 (0xFF24), NR51 (0xFF25), NR52 (0xFF26).

This emulator implements Channels 1 & 2 (pulse waves with duty cycle, envelope, and sweep).

---

## 9. Save System
- **Battery saves (.sav):** For cartridges with battery-backed SRAM (types 0x03, 0x06, 0x09, 0x0D, 0x0F, 0x10, 0x13, 0x1B, 0x1E), the external RAM is saved to a `.sav` file alongside the ROM. Saves are written on exit and when pressing F5.
- **Save states:** Press **F5** to save state, **F8** to load state. States are stored in `.state` files.

---

## 10. Controls
| Key     | Action      |
|---------|-------------|
| Arrows  | D-Pad       |
| A       | A Button    |
| S       | B Button    |
| Space   | Select      |
| Enter   | Start       |
| Tab     | Turbo (hold)|
| F5      | Save state  |
| F8      | Load state  |

---

## 11. Game Boy Color (GBC) — What's Needed
To run GBC games like Pokémon Gold, the following changes are required:

### 11.1 WRAM Banking
- GBC has 8 banks of 4KB WRAM (32KB total vs 8KB on DMG)
- Bank 0 is fixed at 0xC000-0xCFFF
- Banks 1-7 switchable at 0xD000-0xDFFF via register FF70

### 11.2 VRAM Banking
- Two VRAM banks (8KB each) switchable via register FF4F
- Bank 1 stores tile attributes (priority, palette, flip, VRAM bank)

### 11.3 Color Palettes
- 8 background palettes + 8 sprite palettes, each with 4 colors (15-bit RGB)
- Accessed via BCPS/BCPD (0xFF68/0xFF69) and OCPS/OCPD (0xFF6A/0xFF6B)
- Replaces the monochrome BGP/OBP0/OBP1 system

### 11.4 Double Speed Mode
- CPU can run at 2x speed (8.4 MHz) via KEY1 register (0xFF4D)
- Timer and PPU timing must be adjusted accordingly

### 11.5 HDMA/GDMA
- New DMA modes via registers FF51-FF55
- General Purpose DMA (GDMA): bulk copy
- HBlank DMA (HDMA): copy 16 bytes per HBlank

### 11.6 PPU Changes
- Tile attributes in VRAM bank 1 (horizontal/vertical flip, priority, palette, bank select)
- Full rewrite of background/sprite rendering to use color palettes

### 11.7 Detection
- CGB flag at ROM[0x0143]: 0x80 = CGB compatible, 0xC0 = CGB only
- Emulator should check this flag and enable GBC mode accordingly

**Estimated effort:** 500+ lines of new code across memory, PPU, and CPU subsystems.

PPU (Pixel Processing Unit) — vram.c + ppu.c

  The Game Boy screen is 160×144 pixels. The PPU doesn't draw arbitrary pixels — it composes a frame from three layers: background, window, and sprites.

  How tiles work (the fundamental building block)

  All graphics are made of 8×8 pixel tiles. Each tile is 16 bytes in VRAM. Each row of 8 pixels takes 2 bytes — a "low byte" and a "high byte". For each
  pixel, you combine 1 bit from each byte to get a 2-bit color ID (0–3):

   byte1 = 0b01001010    (low bits)
   byte2 = 0b10001100    (high bits)

   Pixel 0: high bit 1, low bit 0 → color 2
   Pixel 1: high bit 0, low bit 1 → color 1
   ...etc (read left-to-right = bit 7 down to bit 0)

  This is what lines 41–45 of vram.c do:

   uint8_t byte1 = cpu->memory[tile_data_addr + (bg_y % 8) * 2];     // low byte
   uint8_t byte2 = cpu->memory[tile_data_addr + (bg_y % 8) * 2 + 1]; // high byte
   int bit = 7 - (bg_x % 8);
   uint8_t color_id = ((byte2 >> bit) & 0x1) << 1 | ((byte1 >> bit) & 0x1);

  The 2-bit color ID (0–3) is then passed through the BGP palette register (0xFF47), which maps each ID to one of 4 actual shades. BGP is an 8-bit
  register where bits 1-0 = color for ID 0, bits 3-2 = color for ID 1, etc:

   uint8_t actual_color = (bgp >> (color_id * 2)) & 0x03;
   pixels[y * 160 + x] = palette[actual_color];

  The palette[] array at the top maps those 4 shades to actual ARGB colors for SDL (white, light green, dark green, black).

  Background layer (render_background)

  The background is a 256×256 pixel virtual map (32×32 tiles). The visible 160×144 window scrolls over it using SCX (0xFF42) and SCY (0xFF43). Since
  coordinates are uint8_t, they wrap around at 256 — the background seamlessly tiles.

  The tile map lives at 0x9800 or 0x9C00 (selected by LCDC bit 3). It's a 32×32 grid of tile IDs — each byte is an index saying "draw tile #N here." The
  actual tile pixel data lives at 0x8000 or 0x8800 (LCDC bit 4). With 0x8800 mode, tile IDs are treated as signed (-128 to 127), so tile 0 maps to address
  0x9000.

  For every screen pixel (x, y):

   1. Add scroll offset: bg_x = x + SCX, bg_y = y + SCY
   2. Find which tile: tile_map_addr = map_base + (bg_y/8)*32 + (bg_x/8)
   3. Read tile ID from the map
   4. Find tile data address from the ID
   5. Read the 2 bytes for the specific row (bg_y % 8)
   6. Extract the specific pixel (bg_x % 8)
   7. Apply palette → write to pixel buffer

  Window layer (render_window)

  The window is like a second background layer but doesn't scroll — it's fixed at position (WX-7, WY). It overlays the background. Games use it for HUDs,
  menus, text boxes. It uses its own tile map (LCDC bit 6 selects 0x9800 or 0x9C00) but shares the tile data area with the background.

  The logic is identical to background rendering, but coordinates are relative to (WX-7, WY) instead of (SCX, SCY), and pixels before WY/WX are skipped.

  Sprites (render_sprites)

  Sprites are independent moving objects (characters, items, projectiles). They're defined in OAM (Object Attribute Memory) at 0xFE00–0xFE9F: 40 sprites,
  4 bytes each:

  ┌──────┬──────────────────────────────────────────────────────────────────────────────────────────────┐
  │ Byte │ Purpose                                                                                      │
  ├──────┼──────────────────────────────────────────────────────────────────────────────────────────────┤
  │ 0    │ Y position (on-screen Y + 16, so Y=16 means top of screen)                                   │
  ├──────┼──────────────────────────────────────────────────────────────────────────────────────────────┤
  │ 1    │ X position (on-screen X + 8, so X=8 means left edge)                                         │
  ├──────┼──────────────────────────────────────────────────────────────────────────────────────────────┤
  │ 2    │ Tile ID (index into tile data at 0x8000, always unsigned)                                    │
  ├──────┼──────────────────────────────────────────────────────────────────────────────────────────────┤
  │ 3    │ Attributes: bit 4 = palette (OBP0 or OBP1), bit 5 = X flip, bit 6 = Y flip, bit 7 = priority │
  └──────┴──────────────────────────────────────────────────────────────────────────────────────────────┘

  Sprites can be 8×8 or 8×16 (LCDC bit 2). In 8×16 mode, the tile ID's lowest bit is forced to 0 — the sprite uses two consecutive tiles.

  Color ID 0 is transparent for sprites — this is why if (color_id == 0) continue; on line 149. This lets sprites have non-rectangular shapes.

  Sprites use separate palettes: OBP0 (0xFF48) and OBP1 (0xFF49), selected by attribute bit 4.

  PPU timing (update_graphics)

  The real Game Boy PPU cycles through 4 modes per scanline, taking exactly 456 CPU cycles per line, and 154 lines per frame (144 visible + 10 VBlank):

  ┌────────────────────┬───────────────┬───────────────────────────────────────────┐
  │ Mode               │ Cycles        │ What happens                              │
  ├────────────────────┼───────────────┼───────────────────────────────────────────┤
  │ 2 (OAM scan)       │ 0–80          │ PPU searches OAM for sprites on this line │
  ├────────────────────┼───────────────┼───────────────────────────────────────────┤
  │ 3 (Pixel transfer) │ 80–252        │ PPU reads VRAM and pushes pixels to LCD   │
  ├────────────────────┼───────────────┼───────────────────────────────────────────┤
  │ 0 (HBlank)         │ 252–456       │ Line done, CPU can access VRAM freely     │
  ├────────────────────┼───────────────┼───────────────────────────────────────────┤
  │ 1 (VBlank)         │ Lines 144–153 │ Frame done, all 456 cycles are VBlank     │
  └────────────────────┴───────────────┴───────────────────────────────────────────┘

  Your update_graphics accumulates CPU cycles in cycle_count. Each iteration:

   1. Determines the current mode based on cycle_count within the line and current LY
   2. If mode changed, optionally fires a STAT interrupt (bit 1 of IF) if the game enabled it in the STAT register
   3. Checks LYC==LY coincidence — games use this to trigger effects at specific scanlines
   4. When cycle_count >= 456: advance LY, subtract 456 cycles
   5. When LY hits 144: fire VBlank interrupt (bit 0 of IF)

  The main loop in main.c detects the VBlank moment (ly == 144 && last_ly != 144) to call update_display(), which renders the full frame to SDL. Your
  emulator renders the whole frame at VBlank rather than per-scanline — simpler but less accurate.

  Display output (update_display)

   render_background(cpu, screen_pixels);  // Draw 256×256 BG, scrolled to 160×144
   render_window(cpu, screen_pixels);      // Overlay window on top
   render_sprites(cpu, screen_pixels);     // Draw sprites on top of that
   SDL_UpdateTexture(texture, ...);        // Copy pixel buffer to GPU texture
   SDL_RenderCopy(renderer, ...);          // Scale 160×144 up to 640×576 window
   SDL_RenderPresent(renderer);            // Flip to screen

  The order matters: background first, window overlays it, sprites overlay everything. This is why the pixels[] array is written to three times — each
  layer overwrites the previous.

  -------------------------------------------------------------------------------------------------------------------------------------------------------

  APU (Audio Processing Unit) — apu.c

  The Game Boy has 4 sound channels. Your emulator implements channels 1 and 2 (pulse waves).

  How pulse waves work

  A pulse wave alternates between "high" and "low" at a certain frequency. The duty cycle controls the ratio of high-to-low time. The Game Boy supports 4
  duty cycles:

   12.5%: ________X    (1 high out of 8 steps)
   25%:   X_______X    (2 high out of 8 steps)
   50%:   X____XXX     (3 high out of 8 steps, but shifted)
   75%:   _XXXXXXX_    (inverted 25%)

  This is the duty_table[4][8] — each row is one duty pattern, each column is one of 8 positions. The channel steps through positions at a rate determined
  by its frequency register (11-bit value, 0–2047).

  The period between steps is: (2048 - freq) × 4 / 4194304 seconds. Higher freq value = higher pitch. This is what tick_channel does — it accumulates
  elapsed time (dt) and advances duty_pos each period.

  Sampling process (sample_channel)

  At any moment, the output is simple:

   duty_table[ch->duty][ch->duty_pos] ? ch->volume : -ch->volume;

  If the current duty position is "high" (1), output +volume. If "low" (0), output -volume. Volume ranges 0–15.

  Envelope

  The envelope automatically ramps volume up or down over time. Configured by:

   - volume_init: starting volume (0–15)
   - envelope_dir: 1 = increase, 0 = decrease
   - envelope_period: speed (0 = disabled, 1–7 = steps per 1/64 second)

  Every envelope_period / 64 seconds, volume increases or decreases by 1. This creates the "attack-decay" effect — a note starts loud and fades, or starts
  quiet and swells.

  Sweep (channel 1 only)

  Sweep automatically shifts the frequency over time — creating rising or falling pitch effects (like the "ding-ding-ding" sound when you open a menu in
  Pokémon).

   - sweep_period: how often to shift (in 1/128 second steps)
   - sweep_shift: how much to shift frequency by (freq >> shift)
   - sweep_dir: 0 = increase frequency (pitch up), 1 = decrease (pitch down)

  Every sweep step: new_freq = freq ± (freq >> shift). If the new frequency exceeds 2047, the channel is disabled (overflow protection).

  Length counter

  When enabled, the channel automatically shuts off after (64 - length) / 256 seconds. Used for short sound effects — a beep that should last exactly N
  frames.

  How game code produces sound

  The game writes to NR1x registers (0xFF10–0xFF14) to configure channel 1:

   1. Write sweep params to NR10 (0xFF10)
   2. Write duty + length to NR11 (0xFF11)
   3. Write volume + envelope to NR12 (0xFF12)
   4. Write frequency low 8 bits to NR13 (0xFF13)
   5. Write frequency high 3 bits + trigger bit to NR14 (0xFF14)

  The trigger bit (bit 7 of NR14) is the "play" button — writing 0x80 starts the note. Your apu_write catches this and resets all timers, sets volume to
  initial, enables the channel.

  Routing to SDL

  update_audio() is called every CPU instruction with the cycle count. It converts cycles to seconds (dt = cycles / 4194304), ticks all channel timers,
  then downsamples to 44100 Hz:

  The Game Boy CPU runs at ~4.19 MHz, but your sound card expects 44100 samples/second. So update_audio accumulates time and every 1/44100 seconds, it
  samples both channels, mixes them according to NR51 (which channel goes to which speaker — left/right panning), scales by master volume (NR50), and
  pushes stereo int16 samples into a buffer.

  When the buffer fills (1024 stereo samples), it's queued to SDL with SDL_QueueAudio. SDL's audio thread pulls from this queue and sends it to your sound
  card. The "don't queue if too much buffered" check prevents latency from growing if the emulator runs faster than real-time (turbo mode).

  What's missing

  Channels 3 (wave — plays arbitrary 4-bit waveforms from wave RAM at 0xFF30–0xFF3F) and 4 (noise — pseudo-random for percussion/explosions) aren't
  implemented. The music will sound incomplete — you'll hear melodies but miss bass lines and drums.

  -------------------------------------------------------------------------------------------------------------------------------------------------------

  Save System — save.c

  Battery saves (.sav files)

  On the real cartridge, games like Pokémon have a battery-backed SRAM chip. When you save in-game, the game writes your progress to external RAM
  (0xA000–0xBFFF). The battery keeps this RAM powered when the Game Boy is off.

  Your emulator simulates this with .sav files:

   1. init_save(): Reads ROM header byte 0x0149 to determine RAM size (e.g., 32KB for Pokémon). Checks if the cartridge type has a battery 
  (cart_has_battery). If a .sav file exists next to the ROM, loads it into cpu->external_ram — restoring your save.
   2. write_save(): Dumps cpu->external_ram to the .sav file. Called on exit (SDL_QUIT in update_input) and when pressing F5. The .sav format is just raw 
  bytes — identical to what's on the cartridge SRAM. This means .sav files are compatible with other emulators.

  Save states (.state files)

  Unlike battery saves (which only save what the game chose to write to SRAM), save states capture the entire emulator state — a snapshot you can restore
  at any point.

  save_state(): Writes the entire cpu_t struct to a binary file (all registers, PC, SP, flags, the full 64KB memory array, MBC bank state, halted flag —
  everything). Then appends external RAM if present.

  load_state(): This is the tricky part. It reads the struct back, but there's a problem: the cpu_t contains pointers (rom, external_ram). These point to 
  malloc'd memory whose addresses are different each run. So before overwriting the struct, it saves the current pointer values, reads the file into the
  struct (overwriting the stale serialized pointers), then restores the live pointers. The external RAM content is then loaded separately from the file.

  The effect: pressing F8 teleports the entire emulator back to exactly the state it was in when you pressed F5.

  -------------------------------------------------------------------------------------------------------------------------------------------------------

  How the main loop ties it all together (main.c)

   for (;;) {
       1. Handle delayed EI (interrupt enable takes 1 instruction to activate)
       2. If halted → check if any interrupt is pending to wake up
          If not halted → execute one CPU instruction (returns cycle count)
       3. update_timers(cycles)    — tick DIV/TIMA hardware timers
       4. update_graphics(cycles)  — advance PPU state machine, fire VBlank/STAT interrupts
       5. update_audio(cycles)     — generate sound samples
       6. handle_interrupts()      — if IME=1 and any IF&IE bit set, dispatch to vector
       7. If LY just hit 144 (VBlank edge):
          - Render full frame to SDL
          - Poll input (keyboard → joypad state, F5/F8/Tab)
          - Frame rate limit (sleep until 16.74ms per frame = ~59.73 FPS)
   }

  Every iteration = one CPU instruction (~4 cycles). At 4.19 MHz / 59.73 FPS, that's roughly 70,224 cycles per frame, or about 17,556 instructions. The c
  variable carries cycle count from instruction execution to all the timing subsystems so they stay synchronized.

  The frame limiter uses SDL_GetPerformanceCounter for sub-millisecond precision — without it, the emulator would run as fast as your CPU allows
  (thousands of FPS). Turbo mode (Tab held) simply skips this SDL_Delay, letting everything run uncapped.
