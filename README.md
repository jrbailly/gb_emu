## GameBoy Emulator
This emulator is designed to replicate the functionality of the original GameBoy consoles.

It currently supports only the DMG (Dot Matrix Game) models and MBC1 (Memory Bank Controller 1) cartridges.

The project is under development and aims to provide an authentic retro gaming experience.

It can actually run : Super Mario Land, Tetris , Mario's Picross, Legend of Zelda The - Link's Awakening 

### Keybinding
- A = Left ctrl
- B = Left alt
- start = Enter
- select = Backspace
- F1 = Save current state
- F2 = Load save state

### Dependencies
To compile this project, ensure you have the following tools and libraries installed on your system:
- gcc or clang
- SDL3
- nlohmann_json
- zip
- Google Test
- CMake

### Compilation
    git clone https://github.com/jrbailly/gb_emu.git
    cd gb_emu
    mkdir build
    cd build
    cmake ..
    make

## Tests

Unit tests are implemented for the following modules: CPU, cartridge, controllers, timer.

### Test ROMs

- **[Blargg's test ROMs](https://github.com/crzysdrs/blarggs-test-roms)**:
  - `cpu_instrs`: validates all CPU instructions against real hardware behaviour
  - `instr_timing`: validates T-cycle counts for all CPU instructions
- **[dmg-acid2](https://github.com/mattcurrie/dmg-acid2)** (mattcurrie): passes the PPU rendering accuracy test

## Project Status
This project is currently under development.

## License
Distributed under the MIT License. See LICENSE.txt for more information.