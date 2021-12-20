#include "ppu.h"
#include "cpu.h"
#include <stdio.h>
#include <string.h>

// not doing this shit again
const uint32_t pallete[64] =
    {
        0x545454FF, 0x001e74FF, 0x081090FF, 0x300088FF, 0x440064FF, 0x5c0030FF, 0x540400FF, 0x3c1800FF, 0x202a00FF, 0x083a00FF, 0x004000FF, 0x003c00FF, 0x00323cFF, 0x000000FF, 0x000000FF, 0x000000FF,
        0x989698FF, 0x084cc4FF, 0x3032ecFF, 0X5c1ee4FF, 0x8814b0FF, 0xa01464FF, 0x982220FF, 0x783c00FF, 0x545a00FF, 0x287200FF, 0x087c00FF, 0x007628FF, 0x006678FF, 0x000000FF, 0x000000FF, 0x000000FF,
        0xeceeecFF, 0x4c9aecFF, 0x787cecFF, 0xb062ecff, 0xe454ecff, 0xec58b4ff, 0xec6a64ff, 0xd48820ff, 0xa0aa00ff, 0x74c400ff, 0x4cd020ff, 0x38cc6cff, 0x38b4ccff, 0x3c3c3cff, 0x000000FF, 0x000000FF,
        0xeceeecff, 0xa8ccecff, 0xbcbcecff, 0xd4b2ecff, 0xecaeecff, 0xecaed4ff, 0xecb4b0ff, 0xe4c490ff, 0xccd278ff, 0xb4de78ff, 0xa8e290ff, 0x98e2b4ff, 0xa0d6e4ff, 0xa0a2a0ff, 0x000000FF, 0x000000FF};

// gets ride of some bitwise fuckery
u8 attr_table_lut[] =
    {
        0, 0, 2, 2,
        0, 0, 2, 2,
        4, 4, 6, 6,
        4, 4, 6, 6};

void ppu_init(struct nes *nes)
{
    memset(&nes->ppu, 0, sizeof(nes->ppu) * sizeof(u8));
    nes->ppu.chr_rom = nes->cart.chr_rom;
    nes->ppu.scanline = 240;

    // First Vram page = 0x0000
    // Second Vram Page = 0x0400
    if (nes->cart.header.flags_6 & 0x01)
    {
        // Base nametable address
        // (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
        //(Vertical mirroring: $2000 equals $2800 and $2400 equals $2C00 (e.g. Super Mario Bros.))
        nes->ppu.nametable[0] = (union nametable *)&nes->ppu.vram[0x0];    // $2000
        nes->ppu.nametable[1] = (union nametable *)&nes->ppu.vram[0x0400]; // $2400
        nes->ppu.nametable[2] = (union nametable *)&nes->ppu.vram[0x0];    // $2800
        nes->ppu.nametable[3] = (union nametable *)&nes->ppu.vram[0x0400]; // $2C00
    }
    else
    {
        // Base nametable address
        // (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
        // Horizontal mirroring: $2000 equals $2400 and $2800 equals $2C00 (e.g. Kid Icarus)
        nes->ppu.nametable[0] = (union nametable *)&nes->ppu.vram[0x0];   // $2000
        nes->ppu.nametable[1] = (union nametable *)&nes->ppu.vram[0x0];   // $2400
        nes->ppu.nametable[2] = (union nametable *)&nes->ppu.vram[0x400]; // $2800
        nes->ppu.nametable[3] = (union nametable *)&nes->ppu.vram[0x400]; // $2C00
    }
    nes->ppu.scanline = 0;
    nes->ppu.cycles = 0;

    SDL_Init(SDL_INIT_VIDEO);
    nes->ppu.window = SDL_CreateWindow("desuNES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256 * 2, 240 * 2, SDL_WINDOW_SHOWN);
    nes->ppu.renderer = SDL_CreateRenderer(nes->ppu.window, -1, SDL_RENDERER_ACCELERATED);
    nes->ppu.texture = SDL_CreateTexture(nes->ppu.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 256, 240);
}

void ppu_write(struct nes *nes, u16 addr, u8 value)
{
    switch (addr & 0x7)
    {
    // case 4:
    //     nes->ppu.registers.OAMDATA = value;
    //     nes->ppu.registers.OAMADDR++;
    //     break;
    case 0:
        if ((nes->ppu.registers.PPUSTATUS & PPUSTATUS_VBLANK) && (value & 0x80))
        {
            // check this implementation later
            cpu_interrupt(nes, NMI_VECTOR);
        }

        nes->ppu.registers.PPUCTRL = value;
        //    t: ...GH.. ........ <- d: ......GH
        //    <used elsewhere> <- d: ABCDEF..
        nes->ppu.scroll.t = (nes->ppu.scroll.t & ~VT_NAMETABLE_SEL) | (nes->ppu.registers.PPUCTRL & PPUCTRL_NAMETABLE) << 10;

        // update pointers for backgorund and sprites
        nes->ppu.bg_tile = (struct tile *) &nes->ppu.chr_rom[(0x1000 * ((nes->ppu.registers.PPUCTRL & PPUCTRL_BG_TABLE) >> 4))];
        nes->ppu.sprt_tile = (struct tile *) &nes->ppu.chr_rom[(0x1000 * ((nes->ppu.registers.PPUCTRL & PPUCTRL_SPRITE_TABLE) >> 3))];
        break;
    case 1:
        nes->ppu.registers.PPUMASK = value;
        break;
    case 3:
        nes->ppu.registers.OAMADDR = value;
        break;
    case 5:
        // yyy NN YYYYY XXXXX
        // ||| || ||||| +++++-- coarse X scroll
        // ||| || +++++-------- coarse Y scroll
        // ||| ++-------------- nametable select
        // +++----------------- fine Y scroll

        if (!nes->ppu.scroll.w) // w is not set
        {
            // $2005 first write (w is 0)
            // t: ....... ...ABCDE <- d: ABCDE...
            // x:              FGH <- d: .....FGH
            // w:                  <- 1
            nes->ppu.scroll.fine_x = (value & 0b111);
            nes->ppu.scroll.t = (nes->ppu.scroll.t & ~VT_COARSE_X) | (value >> 3);
            nes->ppu.scroll.w = true;
        }
        else
        {
            // $2005 second write (w is 1)
            // t: FGH..AB CDE..... <- d: ABCDEFGH
            // w:                  <- 0
            // printf("Value: %02x T: %04x \n", value, nes->ppu.scroll.t);
            nes->ppu.scroll.t = (nes->ppu.scroll.t & ~VT_FINE_Y) | ((value & 0b111) << 12);
            nes->ppu.scroll.t = (nes->ppu.scroll.t & ~VT_COARSE_Y) | (value & 0b11111000) << 5;
            nes->ppu.scroll.w = false;
        }
        break;
    case 6:
        if (!nes->ppu.scroll.w)
        {
            // t: .CDEFGH ........ <- d: ..CDEFGH
            // <unused>     <- d: AB......
            // t: Z...... ........ <- 0 (bit Z is cleared)
            // w:                  <- 1
            // check this line later
            nes->ppu.scroll.t = (nes->ppu.scroll.t & ~0x3F00) | (value & 0b00111111) << 8;
            nes->ppu.scroll.t = (nes->ppu.scroll.t & ~0x4000);
            nes->ppu.scroll.w = true;
        }
        else
        {
            //  t: ....... ABCDEFGH <- d: ABCDEFGH
            // v: <...all bits...> <- t: <...all bits...>
            // w:                  <- 0
            nes->ppu.scroll.t = (nes->ppu.scroll.t & ~0x00FF) | value;
            nes->ppu.scroll.v = nes->ppu.scroll.t;
            nes->ppu.scroll.w = false;
        }
        break;
    case 7:
        // Vertical mirroring: $2000 equals $2800 and $2400 equals $2C00 (e.g. Super Mario Bros.))
        // Horizontal mirroring: $2000 equals $2400 and $2800 equals $2C00 (e.g. Kid Icarus)
        if (nes->ppu.scroll.v < 0x3F00)
        {
            nes->ppu.nametable[(nes->ppu.scroll.v & 0x1FFF) / 0x400]->bytes[(nes->ppu.scroll.v & 0x1FFF) % 0x400] = value;
        }
        else
            switch (nes->ppu.scroll.v & 0x1F)
            {
            case 0x10:
            case 0x14:
            case 0x18:
            case 0x1C:
                nes->ppu.pallete_ram[nes->ppu.scroll.v & 0x0F] = value;
            default:
                nes->ppu.pallete_ram[nes->ppu.scroll.v & 0x1F] = value;
            }

        nes->ppu.scroll.v = (nes->ppu.registers.PPUCTRL & PPUCTRL_VRAM_INC) ? nes->ppu.scroll.v + 32 : nes->ppu.scroll.v + 1;
        break;
    default:
        printf("Unimplemented PPU WRITE: %02x\n", addr & 0xF);
        nes->cpu.uoc = true;
    }
}

u8 ppu_read(struct nes *nes, u16 addr)
{
    u8 ret_val;
    switch (addr & 0xF)
    {
    case 2:
        ret_val = nes->ppu.registers.PPUSTATUS;
        nes->ppu.registers.PPUSTATUS &= ~PPUSTATUS_VBLANK;
        nes->ppu.scroll.w = false;
        return (ret_val);
    case 7:
        if (nes->ppu.scroll.v < 0x2000)
        {
            ret_val = nes->ppu.data_buf;
            nes->ppu.data_buf = nes->ppu.chr_rom[nes->ppu.scroll.v & 0x1FFF];
        }
        else if (nes->ppu.scroll.v < 0x3F00)
        {
            ret_val = nes->ppu.data_buf;
            nes->ppu.data_buf = nes->ppu.nametable[(nes->ppu.scroll.v & 0x1FFF) / 0x400]->bytes[(nes->ppu.scroll.v & 0x1FFF) % 0x400];
        }
        else
            switch (nes->ppu.scroll.v & 0x1F)
            {
            case 0x10:
            case 0x14:
            case 0x18:
            case 0x1C:
                ret_val = nes->ppu.pallete_ram[nes->ppu.scroll.v & 0x0F];
            default:
                ret_val = nes->ppu.pallete_ram[nes->ppu.scroll.v & 0x1F];
            }
        nes->ppu.scroll.v = (nes->ppu.registers.PPUCTRL & PPUCTRL_VRAM_INC) ? nes->ppu.scroll.v + 32 : nes->ppu.scroll.v + 1;
        return ret_val;
    default:
        printf("\nUnimplemented PPU READ: %02x\n", addr);
        nes->cpu.uoc = true;
    }
    return 0;
}

void ppu_tick(struct nes *nes)
{
    // The ppu Renders 262 scanlines per frame
    // Each scanline lasts 341 ppu cycles

    // The PPU contains the following:

    // Background
    // VRAM address, temporary VRAM address, fine X scroll, and first/second write toggle - This controls the addresses that the PPU reads during background rendering. See PPU scrolling.
    // 2 16-bit shift registers - These contain the pattern table data for two tiles.
    // Every 8 cycles, the data for the next tile is loaded into the upper 8 bits of this shift register.
    // Meanwhile, the pixel to render is fetched from one of the lower 8 bits.
    // 2 8-bit shift registers - These contain the palette attributes for the lower 8 pixels of the 16-bit shift register. These registers are fed by a latch which contains the palette attribute for the next tile. Every 8 cycles, the latch is loaded with the palette attribute for the next tile.

    static u16 bg_sr_hi;
    static u16 bg_sr_low;

    static u16 attr_sr_hi;
    static u16 attr_sr_low;

    static u8 bg_latch_hi;
    static u8 bg_latch_low;

    static u8 attr_latch_hi;
    static u8 attr_latch_low;

    static u8 pallete_index;
    static u8 bg_pixel;
    static uint32_t color;

    static u16 nametable_addr;
    static u8 nametable_byte;
    static u16 attr_table_addr;
    static u8 attr_table_byte;

//  Visible scanlines (0-239)
if (nes->ppu.scanline < 240 || (nes->ppu.scanline == 261 && nes->ppu.registers.PPUMASK & PPUMASK_SHOW_BG))
{
    // Cycles 0-256 Data for each bg tile is fetched here
    // Cycles 257-320 Sprites for the Next scanline are fetched here
    // Cycles 321-336 This is where the first two tiles for the next scanline are fetched

    if (nes->ppu.cycles < 256 || (nes->ppu.cycles > 319 && nes->ppu.cycles < 336))
    {
        // Afterwards, the shift registers are shifted once, to the data for the next pixel.

        // Nametable byte
        // Attribute table byte
        // Pattern table tile low
        // Pattern table tile high (+8 bytes from pattern table tile low)

        if (nes->ppu.registers.PPUMASK & PPUMASK_SHOW_BG)
        {
            switch ((nes->ppu.cycles) % 8)
            {
            case 0:
                // the shifters are reloaded every 8 cycles
                bg_sr_hi = (bg_sr_hi & 0x00FF) | bg_latch_hi << 8;
                bg_sr_low = (bg_sr_low & 0x00FF) | bg_latch_low << 8;

                attr_sr_hi = (attr_sr_hi & 0x00FF) | attr_latch_hi << 8;
                attr_sr_low = (attr_sr_low & 0x00FF) | attr_latch_low << 8;

                nametable_addr = (nes->ppu.scroll.v & 0x0FFF);

                break;
            case 1: // fetch nametable byte
                nametable_byte = nes->ppu.vram[nametable_addr];
                break;
            case 2:
                attr_table_addr = 0x03C0 | (nes->ppu.scroll.v & 0x0C00) | ((nes->ppu.scroll.v >> 4) & 0x38) | ((nes->ppu.scroll.v >> 2) & 0x07);
                break;
            case 3: // fetch attr byte
                attr_table_byte = nes->ppu.vram[attr_table_addr];

                u8 y = (((nes->ppu.scroll.v & VT_COARSE_Y) >> 5) & 0x3);
                u8 x = ((nes->ppu.scroll.v & VT_COARSE_X) & 0x3);

                // every attribute table byte controls a 4x4 tile
                //   0   1   2   3
                // .---+---+---+---.
                // |   |   |   |   |  0  00 01
                // + D1-D0 + D3-D2 +     10 11
                // |   |   |   |   |  1
                // +---+---+---+---+
                // |   |   |   |   |  2
                // + D5-D4 + D7-D6 +
                // |   |   |   |   |  3
                // `---+---+---+---'

                u8 pallete2bits = (attr_table_byte >> attr_table_lut[y * 4 + x] & 0b11);
                attr_latch_hi = (pallete2bits >> 0x1) * 255;
                attr_latch_low = (pallete2bits & 0x1) * 255;
                break;
            case 5: // fetch lower 8bits of attr table
                bg_latch_low = nes->ppu.bg_tile[nametable_byte].low[((nes->ppu.scroll.v & VT_FINE_Y) >> 12)];
                break;
            case 7:
                bg_latch_hi = nes->ppu.bg_tile[nametable_byte].hi[((nes->ppu.scroll.v & VT_FINE_Y) >> 12)];

                if ((nes->ppu.scroll.v & VT_COARSE_X) == VT_COARSE_X)
                {                                      // if coarse X == 31
                    nes->ppu.scroll.v &= ~VT_COARSE_X; // coarse X = 0
                    nes->ppu.scroll.v ^= 0x0400;       // switch horizontal nametable
                }
                else
                    nes->ppu.scroll.v += 1; // increment coarse X
                break;
            }

            // i dont like these lines, tryna find a solution for these
            pallete_index = ((attr_sr_low >> (7 - nes->ppu.scroll.fine_x)) & 0x1) | ((attr_sr_hi >> (7 - nes->ppu.scroll.fine_x) & 0x1)) << 1;
            bg_pixel = ((bg_sr_hi >> (7 - nes->ppu.scroll.fine_x)) & 0x1) | ((bg_sr_low >> (7 - nes->ppu.scroll.fine_x) & 0x1)) << 1;

            bg_sr_hi = bg_sr_hi >> 15 | bg_sr_hi << 1;
            bg_sr_low = bg_sr_low >> 15 | bg_sr_low << 1;
            attr_sr_hi = attr_sr_hi >> 15 | attr_sr_hi << 1;
            attr_sr_low = attr_sr_low >> 15 | attr_sr_low << 1;
        }
    }

    if (bg_pixel)
        color = pallete[nes->ppu.palettes.background[pallete_index].color[bg_pixel]];
    else
        color = pallete[nes->ppu.pallete_ram[0]];

    if (nes->ppu.cycles == 256)
    {
        if (nes->ppu.registers.PPUMASK & PPUMASK_SHOW_SPRITES)
        {
            for (int sprite_index = 0; sprite_index < 64; sprite_index++)
            {
                if (nes->ppu.oam.sprite[sprite_index].y_pos > nes->ppu.scanline - 8 && nes->ppu.oam.sprite[sprite_index].y_pos <= nes->ppu.scanline)
                {
                    u8 tile_row_hi;
                    u8 tile_row_low;
                    if (nes->ppu.oam.sprite[sprite_index].attributes & ATTR_FLIP_V)
                    {
                        tile_row_hi = nes->ppu.sprt_tile[nes->ppu.oam.sprite[sprite_index].tile_index].hi[7 - (nes->ppu.scanline - nes->ppu.oam.sprite[sprite_index].y_pos)];
                        tile_row_low = nes->ppu.sprt_tile[nes->ppu.oam.sprite[sprite_index].tile_index].low[7 - (nes->ppu.scanline - nes->ppu.oam.sprite[sprite_index].y_pos)];
                    }
                    else
                    {
                        tile_row_hi = nes->ppu.sprt_tile[nes->ppu.oam.sprite[sprite_index].tile_index].hi[(nes->ppu.scanline - nes->ppu.oam.sprite[sprite_index].y_pos)];
                        tile_row_low = nes->ppu.sprt_tile[nes->ppu.oam.sprite[sprite_index].tile_index].low[(nes->ppu.scanline - nes->ppu.oam.sprite[sprite_index].y_pos)];
                    }
                    for (u8 bit = 0; bit < 8; bit++)
                    {
                        u8 sprite_pixel = (nes->ppu.oam.sprite[sprite_index].attributes & ATTR_FLIP_H) ? ((tile_row_hi >> (bit)) & 0x1) | ((tile_row_low >> (bit)) & 0x1) << 1
                                                                                                       : ((tile_row_hi >> (7 - bit)) & 0x1) | ((tile_row_low >> (7 - bit)) & 0x1) << 1;
                        if (sprite_pixel)
                        {
                            if (sprite_index == 0)
                            {
                                nes->ppu.registers.PPUSTATUS |= PPUSTATUS_0_HIT;
                            }

                            if (!(nes->ppu.oam.sprite[sprite_index].attributes & ATTR_PRIO) || nes->ppu.framebuffer[(nes->ppu.scanline * 256) + (nes->ppu.oam.sprite[sprite_index].x_pos) + bit] == 0x000000FF)
                            {
                                color = pallete[nes->ppu.palettes.sprite[nes->ppu.oam.sprite[sprite_index].attributes & ATTR_PALETTE].color[sprite_pixel]];
                                nes->ppu.framebuffer[(nes->ppu.scanline * 256) + (nes->ppu.oam.sprite[sprite_index].x_pos) + bit] = color;
                            }
                        }
                    }
                }
            }
        }
    }

    if (nes->ppu.cycles < 256 && nes->ppu.scanline < 240)
        nes->ppu.framebuffer[(nes->ppu.scanline * 256) + nes->ppu.cycles] = color;

    // Reset Coarse X based on the T
    if (nes->ppu.cycles == 257 && nes->ppu.registers.PPUMASK & PPUMASK_SHOW_BG)
    {
        nes->ppu.scroll.v = (nes->ppu.scroll.v & ~0b10000011111) | (nes->ppu.scroll.t & 0b10000011111);
    }

    // Increase V every 256 cycles
    if (nes->ppu.cycles == 256 && nes->ppu.registers.PPUMASK & PPUMASK_SHOW_BG)
    {
        if ((nes->ppu.scroll.v & VT_FINE_Y) != VT_FINE_Y) // if fine Y < 7
            nes->ppu.scroll.v += 0x1000;                  // increment fine Y
        else
        {
            nes->ppu.scroll.v &= 0x8FFF;                    // fine Y = 0
            int y = (nes->ppu.scroll.v & VT_COARSE_Y) >> 5; // let y = coarse Y
            if (y == 29)
            {
                y = 0;                       // coarse Y = 0
                nes->ppu.scroll.v ^= 0x0800; // switch vertical nametable
            }
            else if (y == 31)
                y = 0; // coarse Y = 0, nametable not switched
            else
                y += 1;                                                        // increment coarse Y
            nes->ppu.scroll.v = (nes->ppu.scroll.v & ~VT_COARSE_Y) | (y << 5); // put coarse Y back into v
        }
    }
    }

    if (nes->ppu.scanline == 261)
    {
        if (nes->ppu.cycles == 304 && nes->ppu.registers.PPUMASK & PPUMASK_SHOW_BG)
        {
            nes->ppu.scroll.v = (nes->ppu.scroll.v & ~0x7BE0) | (nes->ppu.scroll.t & 0x7BE0);
        }
    }

    nes->ppu.cycles++;

    if (nes->ppu.cycles >= 341)
    {
        nes->ppu.cycles = 0;
        nes->ppu.scanline++;
        // something happens when the scanline increases ?

        //Pre-render scanline (-1 or 261)
        //This is a dummy scanline, whose sole purpose is to fill the shift registers with the data for the first two tiles of the next scanline.
        //Although no pixels are rendered for this scanline, the PPU still makes the same memory accesses it would for a regular scanline.
        if (nes->ppu.scanline == 262)
        {
            nes->ppu.registers.PPUSTATUS &= ~PPUSTATUS_0_HIT;
            nes->ppu.registers.PPUSTATUS &= ~PPUSTATUS_VBLANK;
            nes->ppu.scanline = 0;
            return;
        }

        if (nes->ppu.scanline == 241)
        {
            SDL_UpdateTexture(nes->ppu.texture, NULL, nes->ppu.framebuffer, 256 * sizeof(uint32_t));
            SDL_RenderCopy(nes->ppu.renderer, nes->ppu.texture, NULL, NULL);
            SDL_RenderPresent(nes->ppu.renderer);

            nes->ppu.registers.PPUSTATUS |= PPUSTATUS_VBLANK;

            if (nes->ppu.registers.PPUCTRL & PPUCONTROL_NMI)
            {
                cpu_interrupt(nes, NMI_VECTOR);
            }
        }
    }
}