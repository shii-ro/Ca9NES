#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "nes.h"
#include "cart.h"

void cart_load_header(struct nes *nes, FILE *rom )
{
    fread(&nes->cart.header, sizeof(u8), sizeof(struct header), rom);
    //check INES values
    if (nes->cart.header.constant[0] == 'N' &&
        nes->cart.header.constant[1] == 'E' &&
        nes->cart.header.constant[2] == 'S')
        printf("Valid NES file.\n");

    nes->cart.chr_rom_size = nes->cart.header.chr_rom_size * (8 * 1024);
    nes->cart.prg_rom_size = nes->cart.header.prg_rom_size * (16 * 1024);

    nes->cart.mapper_index = (nes->cart.header.flags_7 & 0xFF) << 8 | ((nes->cart.header.flags_6 >> 8) & 0xFF);
}

void cart_load_rom(struct nes *nes, FILE *rom)
{
    nes->cart.chr_rom = malloc(nes->cart.chr_rom_size);
    nes->cart.prg_rom = malloc(nes->cart.prg_rom_size);

    fread(nes->cart.prg_rom, sizeof(u8), nes->cart.prg_rom_size, rom);
    fread(nes->cart.chr_rom, sizeof(u8), nes->cart.chr_rom_size, rom);
}

void cart_init(struct nes *nes, char *romname)
{
    FILE *rom = fopen(romname, "rb");
    cart_load_header(nes, rom);
    cart_load_rom(nes, rom);
    fclose(rom);

    mapper_init(nes);
}

void cart_close(struct nes *nes)
{
    free(nes->cart.prg_rom);
    free(nes->cart.chr_rom);
}

u8 mapper00_read(struct mapper *mapper, u16 addr)
{
    // reading from  C000 and beyond always have the 15th bit true
    return mapper->prg_rom_bank[(addr >> 14) & 0x1][addr & 0x3FFF];
}

void mapper00_init(struct nes *nes)
{
    
    nes->cart.mapper.prg_rom_bank[0] = nes->cart.prg_rom;
    nes->cart.mapper.prg_rom_bank[1] = ( nes->cart.header.prg_rom_size == 1) ?
                                    nes->cart.prg_rom : 
                                    nes->cart.prg_rom +  0x4000;

    nes->cart.mapper.mapper_read = &mapper00_read;
}

void mapper_init(struct nes *nes)
{
    switch (nes->cart.mapper_index)
    {
    case 0x00: mapper00_init(nes); break;
    default:
        printf("Mapper not implemented: %02x\n", nes->cart.mapper_index);
    }
}
