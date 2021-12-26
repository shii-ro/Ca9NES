#include "../common.h"
#include "../nes.h"

void mapper00_write(struct nes *nes, u16 addr, u8 value)
{

}

u8 mapper00_read(struct nes *nes, u16 addr)
{
    // reading from  C000 and beyond always have the 15th bit true
    return nes->mapper.prg_rom_bank[(addr >> 14) & 0x1][addr & 0x3FFF];
}

void mapper00_init(struct nes *nes)
{

    nes->mapper.prg_rom_bank[0] = nes->cart.prg_rom;
    nes->mapper.prg_rom_bank[1] = (nes->cart.header.prg_rom_size == 1) ? nes->cart.prg_rom : &nes->cart.prg_rom[0x4000];

    nes->mapper.mapper_read = &mapper00_read;
    nes->mapper.mapper_write = &mapper00_write;
}
