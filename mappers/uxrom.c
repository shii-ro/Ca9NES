#include "../common.h"
#include "../nes.h"

u8 uxrom_read(struct nes *nes, u16 addr)
{
    return nes->mapper.prg_rom_bank[(addr >> 14) & 0x1][addr & 0x3FFF];
}

void uxrom_write(struct nes *nes, u16 addr, u8 value)
{
    nes->mapper.prg_rom_bank[0] = &nes->cart.prg_rom[value * 0x4000];
}

void uxrom_init(struct nes *nes)
{
    nes->mapper.prg_rom_bank[0] = nes->cart.prg_rom;
    nes->mapper.prg_rom_bank[1] = &nes->cart.prg_rom[((nes->cart.header.prg_rom_size) - 1) * 0x4000];

    nes->mapper.mapper_read = &uxrom_read;
    nes->mapper.mapper_write = &uxrom_write;
}
