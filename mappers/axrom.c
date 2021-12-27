#include "../common.h"
#include "../nes.h"

void axrom_write(struct nes *nes, u16 addr, u8 value)
{
    if (addr >= 0x8000)
    {
        nes->mapper.prg_rom_bank[0] = &nes->cart.prg_rom[(value & 0x7) * 0x8000];

        if (value & 0x10)
        {
            nes->ppu.nametable[0] = (union nametable *)&nes->ppu.vram[0x400]; // $2000
            nes->ppu.nametable[1] = (union nametable *)&nes->ppu.vram[0x400]; // $2400
            nes->ppu.nametable[2] = (union nametable *)&nes->ppu.vram[0x400]; // $2800
            nes->ppu.nametable[3] = (union nametable *)&nes->ppu.vram[0x400]; // $2C00
        }
        else
        {
            nes->ppu.nametable[0] = (union nametable *)&nes->ppu.vram[0x0];   // $2000
            nes->ppu.nametable[1] = (union nametable *)&nes->ppu.vram[0x000]; // $2400
            nes->ppu.nametable[2] = (union nametable *)&nes->ppu.vram[0x000]; // $2800
            nes->ppu.nametable[3] = (union nametable *)&nes->ppu.vram[0x000]; // $2C00
        }
    }
}

u8 axrom_read(struct nes *nes, u16 addr)
{
    return nes->mapper.prg_rom_bank[0][addr & 0x7FFF];
}

void axrom_init(struct nes *nes)
{

    nes->mapper.prg_rom_bank[0] = &nes->cart.prg_rom[0x0];

    nes->mapper.mapper_read = &axrom_read;
    nes->mapper.mapper_write = &axrom_write;
}
