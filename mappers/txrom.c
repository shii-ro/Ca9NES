#include "../common.h"
#include "../nes.h"

void txrom_write(struct nes *nes, u16 addr, u8 value)
{
    #define CHR_A12_INVERSION 0b10000000 // (0: two 2 KB banks at $0000-$0FFF, four 1 KB banks at $1000-$1FFF;
                                         //  1: two 2 KB banks at $1000-$1FFF,
                                         //  four 1 KB banks at $0000-$0FFF)
    #define PRG_ROM_BANK_MODE 0b01000000 // 0: $8000-$9FFF swappable, $C000-$DFFF fixed to second-last bank
                                         // 1: $C000-$DFFF swappable, $8000-$9FFF fixed to second-last bank)
    #define BANK              0b111

    static u8  bank_select =0;
    static u16 swappable = 0;
    // nes->test_toogle = true;
    // printf("ATTEMPTIMT TO WRITE: ADDR %04x\t", addr);
    switch (addr >> 12)
    {
    case 0x6:
    case 0x7:
        nes->mapper.prg_ram[addr & 0x1FFF] = value;
        break;
    case 0x8:
    case 0x9:
        if(addr & 0x1)
        {
            u8 inversion = (bank_select & CHR_A12_INVERSION) ? 4 : 0;
            switch (bank_select & 0x7)
            {
            case 0: // Select 2 KB CHR bank at PPU $0000-$07FF (or $1000-$17FF)
                nes->ppu.pattern_banks[0 ^ inversion] = &nes->cart.chr_banks[value & 0xFE];
                nes->ppu.pattern_banks[1 ^ inversion] = &nes->cart.chr_banks[(value & 0xFE) + 1];
                break;
            case 1: // Select 2 KB CHR bank at PPU $0800-$0FFF (or $1800-$1FFF)
                nes->ppu.pattern_banks[2 ^ inversion] = &nes->cart.chr_banks[value & 0xFE];
                nes->ppu.pattern_banks[3 ^ inversion] = &nes->cart.chr_banks[(value & 0xFE) + 1];
                break;
            case 2: // Select 1 KB CHR bank at PPU $1000-$13FF (or $0000-$03FF)
                nes->ppu.pattern_banks[4 ^ inversion] = &nes->cart.chr_banks[value];
                break;
            case 3: // Select 1 KB CHR bank at PPU $1400-$17FF (or $0400-$07FF)
                nes->ppu.pattern_banks[5 ^ inversion] = &nes->cart.chr_banks[value];
                break;
            case 4: // Select 1 KB CHR bank at PPU $1800-$1BFF (or $0800-$0BFF)
                nes->ppu.pattern_banks[6 ^ inversion] = &nes->cart.chr_banks[value];
                break;
            case 5: // Select 1 KB CHR bank at PPU $1C00-$1FFF (or $0C00-$0FFF)
                nes->ppu.pattern_banks[7 ^ inversion] = &nes->cart.chr_banks[value];
                break;
            case 6: nes->mapper.prg_rom_bank[swappable] = &nes->cart.prg_rom[(value & 0x3F) * 0x2000]; break;// Select 8 KB PRG ROM bank at $8000-$9FFF (or $C000-$DFFF)
            case 7: nes->mapper.prg_rom_bank[1] = &nes->cart.prg_rom[(value & 0x3F) * 0x2000]; break;// Select 8 KB PRG ROM bank at $A000-$BFFF
            }
            break;
        }
        else
        {
            bank_select = value;
            swappable = (bank_select & PRG_ROM_BANK_MODE) ? 2 : 0;
            nes->mapper.prg_rom_bank[(bank_select & PRG_ROM_BANK_MODE) ? 0 : 2] = &nes->cart.prg_rom[0x2000 * ((nes->cart.header.prg_rom_size * 2) - 2)]; // Second To Last bank
        }
        break;
    case 0xA:
    case 0xB:
        if (addr & 0x1)
            break; // implement PRG RAM protect later (and add open bus)
        if (value & 0x01)
        {
            nes->ppu.nametable[0] = (union nametable *)&nes->ppu.vram[0x0];   // $2000
            nes->ppu.nametable[1] = (union nametable *)&nes->ppu.vram[0x000]; // $2400
            nes->ppu.nametable[2] = (union nametable *)&nes->ppu.vram[0x400]; // $2800
            nes->ppu.nametable[3] = (union nametable *)&nes->ppu.vram[0x400]; // $2C00
        }
        else
        {
            nes->ppu.nametable[0] = (union nametable *)&nes->ppu.vram[0x000]; // $2000
            nes->ppu.nametable[1] = (union nametable *)&nes->ppu.vram[0x400]; // $2400
            nes->ppu.nametable[2] = (union nametable *)&nes->ppu.vram[0x000]; // $2800
            nes->ppu.nametable[3] = (union nametable *)&nes->ppu.vram[0x400]; // $2C00
        }
        break;
    case 0xC:
    case 0xD:
        if (addr & 0x1){
            nes->mapper.irq.reload = true;
            nes->mapper.irq.counter = 0;
        }
        else
            nes->mapper.irq.latch = value;
        break;
    case 0xE:
    case 0xF:
        if (addr & 0x1)
            nes->mapper.irq.enable = true;
        else
            nes->mapper.irq.enable = false;
        break;
    }
}

u8 txrom_read(struct nes *nes, u16 addr)
{
    switch (addr >> 12)
    {
    case 0x6:
    case 0x7:
        return nes->mapper.prg_ram[addr & 0x1FFF];
    case 0x8:
    case 0x9:
        return nes->mapper.prg_rom_bank[0][addr & 0x1FFF];
    case 0xA:
    case 0xB:
        return nes->mapper.prg_rom_bank[1][addr & 0x1FFF];
    case 0xC:
    case 0xD:
        return nes->mapper.prg_rom_bank[2][addr & 0x1FFF];
    case 0xE:
    case 0xF:
        return nes->mapper.prg_rom_bank[3][addr & 0x1FFF];
    }
}

void txrom_init(struct nes *nes)
{
    // CPU $8000-$9FFF (or $C000-$DFFF): 8 KB switchable PRG ROM bank
    // CPU $A000-$BFFF: 8 KB switchable PRG ROM bank
    // CPU $C000-$DFFF (or $8000-$9FFF): 8 KB PRG ROM bank, fixed to the second-last bank
    // CPU $E000-$FFFF: 8 KB PRG ROM bank, fixed to the last bank

    nes->mapper.prg_rom_bank[3] = &nes->cart.prg_rom[0x2000 * ((nes->cart.header.prg_rom_size * 2) - 1)];

    nes->mapper.mapper_read = &txrom_read;
    nes->mapper.mapper_write = &txrom_write;
}