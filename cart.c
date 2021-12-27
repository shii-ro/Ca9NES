#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "nes.h"
#include "cart.h"
#include "mappers/nrom.c"
#include "mappers/sxrom.c"
#include "mappers/uxrom.c"
#include "mappers/txrom.c"
#include "mappers/axrom.c"


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

    if (!nes->cart.header.chr_rom_size)
        nes->mapper.uses_chr_ram = true;
    if (nes->cart.header.flags_6 & 2)
        nes->mapper.uses_prg_ram = true;

    printf("CHR SIZE: %d PRG_SIZE %d\n", nes->cart.chr_rom_size, nes->cart.prg_rom_size);
    printf("MIRRORING: %x ", nes->cart.header.flags_6 & 0x1);

    nes->cart.mapper_index = (nes->cart.header.flags_7 & 0xFF) << 4 | ((nes->cart.header.flags_6 >> 4) & 0xFF);
    printf("MAPPER: %x\n", nes->cart.mapper_index);
}

void cart_load_rom(struct nes *nes, FILE *rom)
{
    
    nes->cart.prg_rom = malloc(nes->cart.prg_rom_size);

    fread(nes->cart.prg_rom, sizeof(u8), nes->cart.prg_rom_size, rom);

    if (nes->mapper.uses_chr_ram)
    {
        printf("USES CHR RAM\n");
        nes->cart.chr_rom = malloc(sizeof(u8) * 0x2000);
    }
    else
    {
        nes->cart.chr_rom = malloc(nes->cart.chr_rom_size);
        fread(nes->cart.chr_rom, sizeof(u8), nes->cart.chr_rom_size, rom);
    }
    
    nes->cart.chr_banks = (struct chr_banks *)&nes->cart.chr_rom[0];
    if (nes->mapper.uses_prg_ram || true)
    {
        printf("Uses PRG RAM\n");
        nes->mapper.prg_ram = malloc(sizeof(u8) * 0x2000);
        nes_read_prg_ram(nes);
    }
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
    if (nes->mapper.uses_prg_ram || true)
        free(nes->mapper.prg_ram);
    free(nes->cart.prg_rom);
    free(nes->cart.chr_rom);
}

void mapper_init(struct nes *nes)
{
    switch (nes->cart.mapper_index)
    {
    case 0x00: nrom_init(nes); break;
    case 0x01: sxrom_init(nes); break;
    case 0x02: uxrom_init(nes); break;
    case 0x04: txrom_init(nes); break;
    case 0x07: axrom_init(nes); break;
    default:
        printf("Mapper not implemented: %02x\n", nes->cart.mapper_index);
        nes->quit = true;
        break;
    }
}
