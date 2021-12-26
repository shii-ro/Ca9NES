#include "ppu.h"
#include "cpu.h"
#include <stdio.h>
#include <string.h>

enum
{
    PPUCTRL,
    PPUMASK,
    PPUSTATUS,
    OAMADDR,
    OAMDATA,
    PPUSCROLL,
    PPUADDR,
    PPUDATA,
} PPU_REGISTERS;

#define VISIBLE_SCANLINE    240
#define PRERENDER_SCANLINE  261
#define VBLANK_SCANLINE     241

static inline u8 *ppu_get_pattern_p(struct nes *nes, u16 addr)
{
    return &nes->ppu.pattern_banks[addr >> 10]->bytes[addr & 0x3FF];
}

// not doing this shit again
const uint32_t pallete[64] =
    {
        0x545454FF, 0x001e74FF, 0x081090FF, 0x300088FF, 0x440064FF, 0x5c0030FF, 0x540400FF, 0x3c1800FF, 0x202a00FF, 0x083a00FF, 0x004000FF, 0x003c00FF, 0x00323cFF, 0x000000FF, 0x000000FF, 0x000000FF,
        0x989698FF, 0x084cc4FF, 0x3032ecFF, 0X5c1ee4FF, 0x8814b0FF, 0xa01464FF, 0x982220FF, 0x783c00FF, 0x545a00FF, 0x287200FF, 0x087c00FF, 0x007628FF, 0x006678FF, 0x000000FF, 0x000000FF, 0x000000FF,
        0xeceeecFF, 0x4c9aecFF, 0x787cecFF, 0xb062ecff, 0xe454ecff, 0xec58b4ff, 0xec6a64ff, 0xd48820ff, 0xa0aa00ff, 0x74c400ff, 0x4cd020ff, 0x38cc6cff, 0x38b4ccff, 0x3c3c3cff, 0x000000FF, 0x000000FF,
        0xeceeecff, 0xa8ccecff, 0xbcbcecff, 0xd4b2ecff, 0xecaeecff, 0xecaed4ff, 0xecb4b0ff, 0xe4c490ff, 0xccd278ff, 0xb4de78ff, 0xa8e290ff, 0x98e2b4ff, 0xa0d6e4ff, 0xa0a2a0ff, 0x000000FF, 0x000000FF};

const u8 attr_table_lut[] =
    {
        0, 0, 2, 2,
        0, 0, 2, 2,
        4, 4, 6, 6,
        4, 4, 6, 6};

void ppu_init(struct nes *nes)
{
    memset(&nes->ppu, 0, sizeof(nes->ppu) * sizeof(u8));
    nes->ppu.scanline = 240;

    for (int i = 0; i < 8; i++)
    {
        nes->ppu.pattern_banks[i] = &nes->cart.chr_banks[i];
    }

    if (nes->cart.header.flags_6 & 0x01)
    {
        nes->ppu.nametable[0] = (union nametable *)&nes->ppu.vram[0x000]; // $2000
        nes->ppu.nametable[1] = (union nametable *)&nes->ppu.vram[0x400]; // $2400
        nes->ppu.nametable[2] = (union nametable *)&nes->ppu.vram[0x000]; // $2800
        nes->ppu.nametable[3] = (union nametable *)&nes->ppu.vram[0x400]; // $2C00
    }
    else
    {
        nes->ppu.nametable[0] = (union nametable *)&nes->ppu.vram[0x0];   // $2000
        nes->ppu.nametable[1] = (union nametable *)&nes->ppu.vram[0x000]; // $2400
        nes->ppu.nametable[2] = (union nametable *)&nes->ppu.vram[0x400]; // $2800
        nes->ppu.nametable[3] = (union nametable *)&nes->ppu.vram[0x400]; // $2C00
    }

    nes->ppu.scanline = 0;
    nes->ppu.cycles = 0;

    nes->ppu.window = SDL_CreateWindow("desuNES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256 * 2, 240 * 2, SDL_WINDOW_SHOWN);
    nes->ppu.renderer = SDL_CreateRenderer(nes->ppu.window, -1, SDL_RENDERER_ACCELERATED);
    nes->ppu.texture = SDL_CreateTexture(nes->ppu.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 256, 240);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
}

void ppu_write(struct nes *nes, u16 addr, u8 value)
{
    switch (addr & 0x7)
    {
    case PPUCTRL:
        if ((nes->ppu.registers.PPUSTATUS & PPUSTATUS_VBLANK) && (value & 0x80) && !(nes->ppu.registers.PPUCTRL & 0x80) && nes->ppu.scanline >= 241)
            cpu_interrupt(nes, NMI_VECTOR);
        nes->ppu.registers.PPUCTRL = value;
        nes->ppu.bg_index = (nes->ppu.registers.PPUCTRL & PPUCTRL_BG_TABLE) ? 0x1000 : 0x000;
        nes->ppu.scroll.t = (nes->ppu.scroll.t & ~VT_NAMETABLE_SEL) | (nes->ppu.registers.PPUCTRL & PPUCTRL_NAMETABLE) << 10;
        break;
    case PPUMASK:
        nes->ppu.registers.PPUMASK = value;
        break;
    case OAMADDR:
        nes->ppu.registers.OAMADDR = value;
        break;
    case OAMDATA:
        nes->ppu.oam.oam_bytes[nes->ppu.registers.OAMADDR++] = value;
        break;
    case PPUSCROLL:
        if (!nes->ppu.scroll.w) // w is not set
        {
            nes->ppu.scroll.fine_x = (value & 0b111);
            nes->ppu.scroll.t = (nes->ppu.scroll.t & ~VT_COARSE_X) | (value >> 3);
            nes->ppu.scroll.w = true;
        }
        else
        {
            nes->ppu.scroll.t = (nes->ppu.scroll.t & ~VT_FINE_Y) | ((value & 0b111) << 12);
            nes->ppu.scroll.t = (nes->ppu.scroll.t & ~VT_COARSE_Y) | (value & 0b11111000) << 2;
            nes->ppu.scroll.w = false;
        }
        break;
    case PPUADDR:
        if (!nes->ppu.scroll.w)
        {
            nes->ppu.scroll.t = (nes->ppu.scroll.t & ~0x3F00) | (value & 0b00111111) << 8;
            nes->ppu.scroll.t = (nes->ppu.scroll.t & ~0x4000);
            nes->ppu.scroll.w = true;
        }
        else
        {
            nes->ppu.scroll.t = (nes->ppu.scroll.t & ~0x00FF) | value;
            nes->ppu.scroll.v = nes->ppu.scroll.t;
            nes->ppu.scroll.w = false;
        }
        break;
    case PPUDATA:
        if (nes->ppu.scroll.v < 0x2000)
        {
            *ppu_get_pattern_p(nes, nes->ppu.scroll.v) = value;
        }
        else if (nes->ppu.scroll.v < 0x3F00)
        {
            nes->ppu.nametable[(nes->ppu.scroll.v & 0xC00) >> 10]->bytes[nes->ppu.scroll.v & 0x3FF] = value;
        }
        else
        {
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
        }

        nes->ppu.scroll.v = (nes->ppu.registers.PPUCTRL & PPUCTRL_VRAM_INC) ? nes->ppu.scroll.v + 32 : nes->ppu.scroll.v + 1;
        break;
    default:
        printf("Unimplemented PPU WRITE: %02x\n", addr & 0xF);
        getchar();
        nes->cpu.uoc = true;
    }
}

u8 ppu_read(struct nes *nes, u16 addr)
{
    u8 ret_val;
    switch (addr & 0xF)
    {
    case PPUSTATUS:
        ret_val = nes->ppu.registers.PPUSTATUS;
        nes->ppu.registers.PPUSTATUS &= ~PPUSTATUS_VBLANK;
        nes->ppu.scroll.w = false;
        return (ret_val);
    case PPUDATA:
        if (nes->ppu.scroll.v < 0x3F00)
        {
            ret_val = nes->ppu.data_buf;
            if (nes->ppu.scroll.v < 0x2000)
                nes->ppu.data_buf =  *ppu_get_pattern_p(nes, nes->ppu.scroll.v);
            else
                nes->ppu.data_buf = nes->ppu.nametable[(nes->ppu.scroll.v & 0xC00) >> 10]->bytes[nes->ppu.scroll.v & 0x3FF];
        }
        else
        {
            switch (nes->ppu.scroll.v & 0x1F)
            {
            case 0x10:
            case 0x14:
            case 0x18:
            case 0x1C:
                ret_val = nes->ppu.pallete_ram[nes->ppu.scroll.v & 0x0F];
                break;
            default:
                ret_val = nes->ppu.pallete_ram[nes->ppu.scroll.v & 0x1F];
                break;
            }
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
    static u16 frame_odd_even = 340;
    static uint32_t bg_sr;
    static u16 bg_pattern;

    static uint32_t attr_sr;
    static uint16_t attr_pattern;

    static u8 bg_latch_hi;
    static u8 bg_latch_low;

    static u8 pallete_index;
    static u8 bg_pixel;
    static uint32_t color;

    static u16 nametable_addr;
    static u8 nametable_byte;
    static u16 attr_table_addr;
    static u8 attr_table_byte;

    static struct sprite *curr_sprite;
    static u8 sprite_counter;
    static u8 oam_counter;
    static u16 sprites[8];
    static struct sprite sec_oam[8];
    static u8 sec_oam_index[8];

    //  Visible scanlines (0-239)
    if ((nes->ppu.scanline < VISIBLE_SCANLINE || nes->ppu.scanline == PRERENDER_SCANLINE) && nes->ppu.registers.PPUMASK & PPUMASK_SHOW_BG)
    {
        if (nes->ppu.cycles < 256 || (nes->ppu.cycles > 319 && nes->ppu.cycles < 336))
        {
            if (nes->ppu.registers.PPUMASK & PPUMASK_SHOW_BG)
            {
                switch ((nes->ppu.cycles) % 8)
                {
                case 0: // fetch nametable addr
                    bg_sr = (bg_sr & 0x0000FFFF) | bg_pattern << 16;
                    attr_sr = (attr_sr & 0x0000FFFF) | attr_pattern << 16;

                    nametable_addr = (nes->ppu.scroll.v & 0x0FFF);
                    break;
                case 1: // fetch nametable byte
                    nametable_byte = nes->ppu.nametable[(nametable_addr >> 10) & 0x3]->byte[nametable_addr & 0x3FF];
                    break;
                case 2: // fetch attr address
                    attr_table_addr = ((nes->ppu.scroll.v >> 4) & 0x38) | ((nes->ppu.scroll.v >> 2) & 0x07);
                    break;
                case 3: // fetch attr byte
                    attr_table_byte = nes->ppu.nametable[(nes->ppu.scroll.v >> 10) & 0x3]->attribute_table[attr_table_addr];
                    break;
                case 4: // fetch attr pattern_banks
                    attr_pattern = (attr_table_byte >> attr_table_lut[(((nes->ppu.scroll.v & VT_COARSE_Y) >> 5) & 0x3) * 4 + ((nes->ppu.scroll.v & VT_COARSE_X) & 0x3)] & 0b11) * 0x5555;
                    break;
                case 5: // fetch lower 8bits of attr table
                    bg_latch_hi = *ppu_get_pattern_p(nes, nes->ppu.bg_index + (nametable_byte * 0x10) + ((nes->ppu.scroll.v & VT_FINE_Y) >> 12));
                    break;
                case 7: // fetch higher 8bits of attr table
                    bg_latch_low = *ppu_get_pattern_p(nes, nes->ppu.bg_index + (nametable_byte * 0x10) + ((nes->ppu.scroll.v & VT_FINE_Y) >> 12) + 8);

                    bg_pattern = bg_latch_hi | bg_latch_low << 8;

                    bg_pattern = (bg_pattern & 0xF00F) | ((bg_pattern & 0x0F00) >> 4) | ((bg_pattern & 0x00F0) << 4);
                    bg_pattern = (bg_pattern & 0xC3C3) | ((bg_pattern & 0x3030) >> 2) | ((bg_pattern & 0x0C0C) << 2);
                    bg_pattern = (bg_pattern & 0x9999) | ((bg_pattern & 0x4444) >> 1) | ((bg_pattern & 0x2222) << 1);

                    bg_pattern = ((bg_pattern >> 2) & 0x3333) | ((bg_pattern & 0x3333) << 2);
                    bg_pattern = ((bg_pattern >> 4) & 0x0F0F) | ((bg_pattern & 0x0F0F) << 4);
                    bg_pattern = ((bg_pattern >> 8) & 0x00FF) | ((bg_pattern & 0x00FF) << 8);

                    if ((nes->ppu.scroll.v & VT_COARSE_X) == VT_COARSE_X)
                    {                                      // if coarse X == 31
                        nes->ppu.scroll.v &= ~VT_COARSE_X; // coarse X = 0
                        nes->ppu.scroll.v ^= 0x0400;       // switch horizontal nametable
                    }
                    else
                        nes->ppu.scroll.v += 1; // increment coarse X
                    break;
                }

                pallete_index = (attr_sr >> (nes->ppu.scroll.fine_x) * 2) & 0x3;
                bg_pixel = (bg_sr >> (nes->ppu.scroll.fine_x * 2)) & 0x3;
                bg_sr >>= 2;
                attr_sr >>= 2;
            }
        }

        if (nes->ppu.cycles == 256 && nes->ppu.scanline < VISIBLE_SCANLINE)
        {
            curr_sprite = &nes->ppu.oam.sprite[0];
            sprite_counter = 0;
            oam_counter = 0;
            for (int sprite = 0; sprite < 64; sprite++)
            {
                if (nes->ppu.scanline >= curr_sprite->y_pos &&
                    nes->ppu.scanline < curr_sprite->y_pos + ((nes->ppu.registers.PPUCTRL & PPUCTRL_SPRITE_SIZE) ? 16 : 8) &&
                    sprite_counter < 8)
                {
                    u8 sprite_row = (nes->ppu.scanline - curr_sprite->y_pos);
                    if ((curr_sprite->attributes & ATTR_FLIP_V))
                        sprite_row = ((nes->ppu.registers.PPUCTRL & PPUCTRL_SPRITE_SIZE) ? 15 : 7) - (sprite_row & 0x7);
                    attr_table_addr = 0x1000 * ((nes->ppu.registers.PPUCTRL & PPUCTRL_SPRITE_SIZE) ? (curr_sprite->tile_index & 0x1) : ((nes->ppu.registers.PPUCTRL & PPUCTRL_SPRITE_TABLE) >> 3));
                    attr_table_addr += 0x10 * ((nes->ppu.registers.PPUCTRL & PPUCTRL_SPRITE_SIZE) ? (curr_sprite->tile_index & 0xFE) : (curr_sprite->tile_index & 0xFF));
                    attr_table_addr += sprite_row & 0x7;
                    if (nes->ppu.scanline > (curr_sprite->y_pos + 7))
                        attr_table_addr += 0x10;

                    u16 sprite_pattern_banks = *ppu_get_pattern_p(nes, attr_table_addr) |  *ppu_get_pattern_p(nes, attr_table_addr + 8) << 8;

                    sprite_pattern_banks = (sprite_pattern_banks & 0xF00F) | ((sprite_pattern_banks & 0x0F00) >> 4) | ((sprite_pattern_banks & 0x00F0) << 4);
                    sprite_pattern_banks = (sprite_pattern_banks & 0xC3C3) | ((sprite_pattern_banks & 0x3030) >> 2) | ((sprite_pattern_banks & 0x0C0C) << 2);
                    sprite_pattern_banks = (sprite_pattern_banks & 0x9999) | ((sprite_pattern_banks & 0x4444) >> 1) | ((sprite_pattern_banks & 0x2222) << 1);

                    if (!(curr_sprite->attributes & ATTR_FLIP_H))
                    {
                        sprite_pattern_banks = ((sprite_pattern_banks >> 2) & 0x3333) | ((sprite_pattern_banks & 0x3333) << 2);
                        sprite_pattern_banks = ((sprite_pattern_banks >> 4) & 0x0F0F) | ((sprite_pattern_banks & 0x0F0F) << 4);
                        sprite_pattern_banks = ((sprite_pattern_banks >> 8) & 0x00FF) | ((sprite_pattern_banks & 0x00FF) << 8);
                    }

                    sec_oam[oam_counter] = *curr_sprite;
                    sec_oam_index[oam_counter++] = sprite;
                    sprites[sprite_counter++] = sprite_pattern_banks;
                }
                curr_sprite++;
            }
        }

        if (nes->ppu.cycles < 256 && nes->ppu.scanline < VISIBLE_SCANLINE)
        {
            color = (bg_pixel) ? pallete[nes->ppu.palettes.background[pallete_index].color[bg_pixel]] : pallete[nes->ppu.pallete_ram[0]];
            if (nes->ppu.registers.PPUMASK & PPUMASK_SHOW_SPRITES && nes->ppu.scanline != 0)
            {
                for (int i = 0; i < oam_counter; i++)
                {
                    u16 x = (nes->ppu.cycles - sec_oam[i].x_pos);
                    if (x >= 8) continue;
                    u8 sprite_pixel = ((sprites[i] >> (x * 2)) & 0x3);
                    if (!sprite_pixel) continue;
                    if (sec_oam_index[i] == 0 && nes->ppu.cycles < 255 && bg_pixel)
                        nes->ppu.registers.PPUSTATUS |= PPUSTATUS_0_HIT;
                    if (!(sec_oam[i].attributes & ATTR_PRIO) || !bg_pixel)
                        color = pallete[nes->ppu.palettes.sprite[sec_oam[i].attributes & ATTR_PALETTE].color[sprite_pixel]];
                    break;
                }
            }
            nes->ppu.framebuffer[(nes->ppu.scanline * 256) + nes->ppu.cycles] = color;
        }

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

    if (nes->ppu.scanline == PRERENDER_SCANLINE && nes->ppu.cycles == 304 && nes->ppu.registers.PPUMASK & PPUMASK_SHOW_BG)
    {
        nes->ppu.scroll.v = (nes->ppu.scroll.v & ~0x7BE0) | (nes->ppu.scroll.t & 0x7BE0);
    }

    nes->ppu.cycles++;

    if (nes->ppu.cycles == 341)
    {
        nes->ppu.cycles = 0;
        nes->ppu.scanline++;
        
        if (nes->ppu.scanline > PRERENDER_SCANLINE)
        {
            nes->ppu.registers.PPUSTATUS &= ~PPUSTATUS_0_HIT;
            nes->ppu.registers.PPUSTATUS &= ~PPUSTATUS_VBLANK;
            nes->ppu.scanline = 0;
            return;
        }

        if (nes->ppu.scanline == VBLANK_SCANLINE)
        {
            SDL_UpdateTexture(nes->ppu.texture, NULL, nes->ppu.framebuffer, 256 * sizeof(uint32_t));
            SDL_RenderCopy(nes->ppu.renderer, nes->ppu.texture, NULL, NULL);
            SDL_RenderPresent(nes->ppu.renderer);

            nes->ppu.registers.PPUSTATUS |= PPUSTATUS_VBLANK;

            if (nes->ppu.registers.PPUCTRL & PPUCTRL_NMI)
            {
                cpu_interrupt(nes, NMI_VECTOR);
            }
        }
    }
}