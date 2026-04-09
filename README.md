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
