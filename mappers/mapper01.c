#include "../common.h"
#include "../nes.h"

u8 mapper01_read(struct nes *nes, u16 addr)
{
    switch (addr >> 13)
    {
    case 3:
        return nes->mapper.prg_ram[addr & 0x1FFF];
    case 4:
    case 5:
        return nes->mapper.prg_rom_bank[0][addr & 0x3FFF];
    case 6:
    case 7:
        return nes->mapper.prg_rom_bank[1][addr & 0x3FFF];
    }
}

void mapper01_write(struct nes *nes, u16 addr, u8 value)
{
    // CPU $8000-$FFFF is connected to a common shift register. Writing a value with bit 7 set
    // ($80 through $FF) to any address in $8000-$FFFF clears the shift register to its initial state.

    static u8 shift_register;
    static u8 shift_register_counter;
    #define CR_MIRRORING 0x3
    #define CR_PRG_BNK_MODE 0xC
    #define CR_CHR_BNK_MODE 0x10
    static u8 control_register;
    static u8 chr_bank0_register;
    static u8 chr_bank1_register;
    static u8 prg_bank_register;

    if (addr < 0x8000)
    {
        nes->mapper.prg_ram[addr & 0x1FFF] = value;
        return;
    }

    if (value & 0x80) // reset shift register to its initial state if true
    {
        shift_register_counter = 0;
        shift_register = 0b10000;
        control_register |= 0xC;
    }
    else
    {
        shift_register = (shift_register >> 1) | (((value & 1) << 4));
        shift_register_counter++;

        if (shift_register_counter == 5)
        {
            // printf("Shift Register Full, value: %02x, addr: %04x\n", shift_register, addr);
            switch (addr >> 13)
            {
            case 4: //$8000
                control_register = shift_register;
                switch (shift_register & 0x3)
                {
                    case 00:
                    case 01:
                        printf("Not implemented mirroring mode\n");
                        nes->cpu.uoc = true;
                        break;
                    case 02:
                        nes->ppu.nametable[0] = (union nametable *)&nes->ppu.vram[0x000]; // $2000
                        nes->ppu.nametable[1] = (union nametable *)&nes->ppu.vram[0x400]; // $2400
                        nes->ppu.nametable[2] = (union nametable *)&nes->ppu.vram[0x000]; // $2800
                        nes->ppu.nametable[3] = (union nametable *)&nes->ppu.vram[0x400]; // $2C00
                        break;
                    case 03:
                        nes->ppu.nametable[0] = (union nametable *)&nes->ppu.vram[0x0];   // $2000
                        nes->ppu.nametable[1] = (union nametable *)&nes->ppu.vram[0x000]; // $2400
                        nes->ppu.nametable[2] = (union nametable *)&nes->ppu.vram[0x400]; // $2800
                        nes->ppu.nametable[3] = (union nametable *)&nes->ppu.vram[0x400]; // $2C00
                        break;
                }
                break;
            case 5: //$A000
                chr_bank0_register = shift_register;
                u8 chr_bank_size = (control_register & CR_CHR_BNK_MODE) ? 4 : 8;
                u8 bank0 = (nes->cart.chr_rom_size == 0 || nes->cart.chr_rom_size == 1) ? (chr_bank0_register & 0x1) : chr_bank0_register * chr_bank_size;
                for (int i = 0; i < chr_bank_size; i++)
                    nes->ppu.pattern_banks[i] = &nes->cart.chr_banks[bank0 + i];
                break;
            case 6: //$C000
                chr_bank1_register = shift_register;
                if (!(control_register & CR_CHR_BNK_MODE)) break;
                for (int i = 0; i < 4; i++)
                    nes->ppu.pattern_banks[i + 4] = &nes->cart.chr_banks[chr_bank1_register * 4 + i];
                break;
            case 7: //$E000
                prg_bank_register = shift_register;
                switch ((control_register & CR_PRG_BNK_MODE) >> 2)
                {
                case 0:
                case 1:
                    // printf("0, 1: switch 32 KB at $8000, ignoring low bit of bank number, rom bank: %d\n", prg_brank_register & 0b1110);
                    nes->mapper.prg_rom_bank[0] = &nes->cart.prg_rom[(prg_bank_register & 0b1110) * 0x4000];
                    nes->mapper.prg_rom_bank[1] = &nes->cart.prg_rom[(prg_bank_register & 0b1111) * 0x4000];
                    break;
                case 2:
                    // printf(" 2: fix first bank at $8000 and switch 16 KB bank at $C000 bank no %d\n", prg_brank_register & 0b1111);
                    nes->mapper.prg_rom_bank[0] = nes->cart.prg_rom;
                    nes->mapper.prg_rom_bank[1] = &nes->cart.prg_rom[(prg_bank_register & 0b1111) * 0x4000];
                    break;
                case 3:
                    // printf("3: fix last bank at $C000 and switch 16 KB bank at $8000 , bank no %d\n", prg_brank_register & 0b1111);
                    nes->mapper.prg_rom_bank[1] = &nes->cart.prg_rom[((nes->cart.header.prg_rom_size) - 1) * 0x4000];
                    nes->mapper.prg_rom_bank[0] = &nes->cart.prg_rom[(prg_bank_register & 0b1111) * 0x4000];
                    break;
                }
                break;
                }

            shift_register = 0b10000;
            shift_register_counter = 0;
        }
    }
    return;
}

void mapper01_init(struct nes *nes)
{
    // CPU $6000-$7FFF: 8 KB PRG RAM bank, (optional)
    // CPU $8000-$BFFF: 16 KB PRG ROM bank, either switchable or fixed to the first bank
    // CPU $C000-$FFFF: 16 KB PRG ROM bank, either fixed to the last bank or switchable
    // PPU $0000-$0FFF: 4 KB switchable CHR bank
    // PPU $1000-$1FFF: 4 KB switchable CHR bank

    nes->mapper.prg_rom_bank[0] = nes->cart.prg_rom;
    nes->mapper.prg_rom_bank[1] = &nes->cart.prg_rom[0x4000 * ((nes->cart.header.prg_rom_size) - 1)];

    nes->mapper.mapper_read = &mapper01_read;
    nes->mapper.mapper_write = &mapper01_write;
}
