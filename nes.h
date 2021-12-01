#ifndef NES_H
#define NES_H

#include "common.h"
#include <stdbool.h>
#include <SDL2/SDL.h>

struct cpu
{
    struct
    {
        u8 a;
        u8 x;
        u8 y;
        u8 s;
        u8 sp;
        u16 pc;
    } registers;
    u8 cycles;
    bool uoc;
};

// VECTORS
#define NMI_VECTOR 0xFFFA
#define RESET_VECTOR 0xFFFC
#define IRQ_BRK_VECTOR 0xFFFE

// FLAGS MAKS
#define STATUS_NEGATIVE 0x80
#define STATUS_OVERFLOW 0x40
#define STATUS_DECIMAL 0x8
#define STATUS_ID 0x4
#define STATUS_ZERO 0x2
#define STATUS_CARRY 0x1



// The PPU addresses a 16kB space, $0000-3FFF, completely separate from the CPU's address bus.
// It is either directly accessed by the PPU itself,or via the CPU with memory mapped registers
// at $2006 and $2007.
struct ppu
{
    union
    {
        u8 ppu_registers[0x9];
        struct
        {
            u8 PPUCTRL;
            u8 PPUMASK;
            u8 PPUSTATUS;
            u8 OAMADDR;
            u8 OAMDATA;
            u8 PPUSCROLL;
            u8 PPUADDR;
            u8 PPUDATA;
            u8 OAMDMA;
        } registers;
    };
    u16 cycles;
    u16 scanline;
    u16 ppu_addr;
    u16 addr_latch;
    u8 *chr_rom;
    u8 vram[0x4000];
    u8 oam[256];

    struct tile
    {
        u8 hi[8];
        u8 low[8];
    } * tile;

    struct nametable
    {
        u8 table[0x3C0];
        u8 attribute_table[64];
    } * nametable;

    struct background_pallete
    {
        u8 color[4];
    } * bg_pallete;

    uint32_t framebuffer[240*256];
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
};

#define PPUSTATUS_VBLANK 0b10000000
#define PPUCONTROL_NMI 0b10000000

struct header
{
    u8 constant[0x4]; // Constant $4E $45 $53 $1A ("NES" followed by MS-DOS end-of-file)
    u8 prg_rom_size;  // Size of PRG ROM in 16 KB units
    u8 chr_rom_size;  // Size of CHR ROM in 8 KB units (Value 0 means the board uses CHR RAM)
    u8 flags_6;       // Mapper, mirroring, battery, trainer
    u8 flags_7;       // Mapper, VS/Playchoice, NES 2.0
    u8 flags_8;       // PRG-RAM size (rarely used extension)
    u8 flags_9;       // TV system (rarely used extension)
    u8 flags_10;      // TV system, PRG-RAM presence (unofficial, rarely used extension)
    u8 unused[0x5];
};

struct mapper
{
    bool uses_prg_ram;
    bool uses_chr_ram;
    u8 *prg_rom_bank[2];
    u8 (*mapper_read)(struct mapper *mapper, u16 addr);
    void (*mapper_write)(struct mapper *mapper, u8 value);
};

struct cart
{
    struct header header;
    u8 *prg_rom;
    u8 *chr_rom;
    int prg_rom_size;
    int chr_rom_size;
    struct mapper mapper;
    u8 mapper_index;
};

struct nes
{
    u8 ram[0x800];
    struct ppu ppu;
    struct cpu cpu;
    struct cart cart;
    long long int total_cycles;
};

u8 nes_read8(struct nes *nes, u16 addr);
void nes_write8(struct nes *nes, u16 addr, u8 value);
void nes_write16(struct nes *nes, u16 addr, u16 value);
void nes_run(struct nes *nes);
void nes_init(struct nes *nes, char *romname);
void nes_close(struct nes *nes);

#endif