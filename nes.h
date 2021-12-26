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
    bool intr_pending[0x3];

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

struct tile
{
    u8 hi[8];
    u8 low[8];
};

struct sprite
{
    u8 y_pos;
    u8 tile_index;
    #define ATTR_FLIP_V 0b10000000
    #define ATTR_FLIP_H 0b01000000
    #define ATTR_PRIO 0b00100000
    #define ATTR_PALETTE 0b00000011
    u8 attributes;
    u8 x_pos;
};

struct chr_banks
{
    u8 bytes[0x400];
};

struct ppu
{
    struct
    {
        #define PPUCTRL_NMI          0b10000000 
        #define PPUCTRL_MASTER_SLAVE 0b01000000
        #define PPUCTRL_SPRITE_SIZE  0b00100000
        #define PPUCTRL_BG_TABLE     0b00010000
        #define PPUCTRL_SPRITE_TABLE 0b00001000
        #define PPUCTRL_VRAM_INC     0b00000100
        #define PPUCTRL_NAMETABLE    0b00000011
        u8 PPUCTRL;
        #define PPUMASK_EMP_BLUE          0b10000000
        #define PPUMASK_EMP_GREEN         0b01000000
        #define PPUMASK_EMP_RED           0b00100000
        #define PPUMASK_SHOW_SPRITES      0b00010000
        #define PPUMASK_SHOW_BG           0b00001000
        #define PPUMASK_SHOW_SPRITES_LEFT 0b00000100
        #define PPUMASK_SHOW_BG_LEFT      0b00000010
        #define PPUMASK_GREYSCALE         0b00000001
        u8 PPUMASK;
        #define PPUSTATUS_VBLANK 0b10000000
        #define PPUSTATUS_0_HIT  0b01000000
        #define PPUSTATUS_OV     0b00100000
        u8 PPUSTATUS;
        u8 OAMADDR;
        u8 OAMDATA;
        u8 PPUSCROLL;
        u8 PPUADDR;
        u8 PPUDATA;
        u8 OAMDMA;
    } registers;

    u16 bg_index;
    u8 data_buf;
    u16 cycles;
    int scanline;
    // u8 *chr_rom;


    // ugly as fuck, lets see if it works
    // define entire pattern acessing as 8 pointers
    struct chr_banks *pattern_banks[8];

    u8 vram[0x800];

    union
    {
        u8 pallete_ram[0x20];
        struct
        {
            // $3F00	Universal background color
            // $3F01-$3F03	Background palette 0
            // $3F05-$3F07	Background palette 1
            // $3F09-$3F0B	Background palette 2
            // $3F0D-$3F0F	Background palette 3
            struct background_palette
            {
                u8 color[4];
            } background[4];

            // $3F11-$3F13	Sprite palette 0
            // $3F15-$3F17	Sprite palette 1
            // $3F19-$3F1B	Sprite palette 2
            // $3F1D-$3F1F	Sprite palette 3
            struct sprite_palette
            {
                u8 color[4];
            } sprite[4];
        } palettes;
    };

    // The OAM (Object Attribute Memory)
    // is internal memory inside the PPU that contains a display list of up to 64 sprites,
    // where each sprite's information occupies 4 bytes.
    union
    {
        u8 oam_bytes[256];
        struct sprite sprite[64];
    } oam;


    // A nametable is a 1024 byte area of memory used by the PPU to lay out backgrounds.
    // Each byte in the nametable controls one 8x8 pixel character cell, and each nametable has 30 rows of 32 tiles each,
    // for 960 ($3C0) bytes; the rest is used by each nametable's attribute table.
    // With each tile being 8x8 pixels, this makes a total of 256x240 pixels in one map, the same size as one full screen.
    union nametable
    {
        u8 bytes[0x400];
        struct
        {
            u8 byte[960];
            u8 attribute_table[64];
        };
    } *nametable[4];

    // v
    // Current VRAM address (15 bits)
    // t
    // Temporary VRAM address (15 bits); can also be thought of as the address of the top left onscreen tile.
    // x
    // Fine X scroll (3 bits)
    // w
    // First or second write toggle (1 bit)

    // The 15 bit registers t and v are composed this way during rendering:

    // yyy NN YYYYY XXXXX
    // ||| || ||||| +++++-- coarse X scroll
    // ||| || +++++-------- coarse Y scroll
    // ||| ++-------------- nametable select
    // +++----------------- fine Y scroll
    // Example 0x2401
    // 010 01 00000 00001
    //  2  1    0   1
    struct
    {
        #define VT_FINE_Y           0x7000
        #define VT_NAMETABLE_SEL    0xC00
        #define VT_COARSE_Y         0x3E0
        #define VT_COARSE_X         0x1F
        u16 v;     // Current VRAM address (15 bits)
        u16 t;     // Temporary VRAM address (15 bits); can also be thought of as the address of the top left onscreen tile.
        u8 fine_x; // Fine X scroll (3 bits)
        bool w;    // First or second write toggle (1 bit)
    } scroll;

    uint32_t framebuffer[240*256];
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
};

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




struct cart
{
    struct header header;
    u8 *prg_rom;
    struct chr_banks *chr_banks;

    u8 *chr_rom; // CHR RAM is always 8kb i presume
    int prg_rom_size;
    int chr_rom_size;
    u8 mapper_index;
};


struct nes
{
    u8 ram[0x800];
    u8 io[0x18];
    char *romname;
    struct ppu ppu;
    struct cpu cpu;
    struct cart cart;

    struct mapper
    {
        bool uses_prg_ram;
        bool uses_chr_ram;
        u8 *prg_ram;
        u8 *chr_ram;
        u8 (*mapper_read)(struct nes *nes, u16 addr);
        void (*mapper_write)(struct nes *nes, u16 addr, u8 value);
        u8 *prg_rom_bank[8];
        struct
        {
            u8 latch;
            bool reload;
            bool enable;
            u8 counter;
        } irq;
    } mapper;

    long long int total_cycles;
    u8 keystate;
    bool test_toogle;
    bool quit;
};

u8 nes_read8(struct nes *nes, u16 addr);
void nes_write8(struct nes *nes, u16 addr, u8 value);
void nes_write16(struct nes *nes, u16 addr, u16 value);
void nes_run(struct nes *nes);
void nes_init(struct nes *nes, char *romname);
void nes_reset(struct nes *nes);
void nes_close(struct nes *nes);
void nes_read_prg_ram(struct nes *nes);
void nes_write_prg_ram(struct nes *nes);
void nes_load_save_state(struct nes *nes);
void nes_write_save_state(struct nes *nes);
#endif