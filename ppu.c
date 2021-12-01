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
    0xeceeecff, 0xa8ccecff, 0xbcbcecff, 0xd4b2ecff, 0xecaeecff, 0xecaed4ff, 0xecb4b0ff, 0xe4c490ff, 0xccd278ff, 0xb4de78ff, 0xa8e290ff, 0x98e2b4ff, 0xa0d6e4ff, 0xa0a2a0ff, 0x000000FF, 0x000000FF
};

void ppu_init(struct nes *nes)
{
    memset(&nes->ppu, 0, sizeof(nes->ppu) * sizeof(u8));
    nes->ppu.chr_rom = nes->cart.chr_rom;
    nes->ppu.tile = (struct tile *)nes->ppu.chr_rom;
    nes->ppu.nametable = (struct nametable *)nes->ppu.vram + 0x2000;
    nes->ppu.bg_pallete = (struct background_pallete * )nes->ppu.vram + 0x3F01;
    nes->ppu.scanline = 0;
    nes->ppu.cycles = 0;
    SDL_Init(SDL_INIT_VIDEO);
    nes->ppu.window = SDL_CreateWindow("desuNES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256, 240, SDL_WINDOW_SHOWN);
    nes->ppu.renderer = SDL_CreateRenderer(nes->ppu.window, -1, SDL_RENDERER_ACCELERATED);
    nes->ppu.texture = SDL_CreateTexture(nes->ppu.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 256, 240);
}

void ppu_render_pixel(struct nes *nes)
{
    //     Conceptually, the PPU does this 33 times for each scanline:

    // Fetch a nametable entry from $2000-$2FBF.
    // Fetch the corresponding attribute table entry from $23C0-$2FFF and increment the current VRAM address within the same row.
    // Fetch the low-order byte of an 8x1 pixel sliver of pattern table from $0000-$0FF7 or $1000-$1FF7.
    // Fetch the high-order byte of this sliver from an address 8 bytes higher.
    // Turn the attribute data and the pattern table data into palette indices, and combine them with data from sprite data using priority.

    u8 nametable_row = (nes->ppu.scanline / 8);

    for (int nametable_column = 0; nametable_column < 32; nametable_column++)
    {
        uint32_t color;
        u16 nametable_index = (nametable_row * 32) + nametable_column;
        u8 nametable_byte = nes->ppu.vram[0x2000 + nametable_index];
        u8 *tile_row_hi = &nes->ppu.tile[0x100 + nametable_byte].hi[(nes->ppu.scanline % 8)];
        u8 *tile_row_low = &nes->ppu.tile[0x100 + nametable_byte].low[(nes->ppu.scanline % 8)];
        u8 attr_byte = nes->ppu.vram[0x23C0 + (((nametable_row / 4) * 8) + (nametable_column / 4))];
        u8 *bg_pallete;
        u8 pallete_index;

        pallete_index = (attr_byte >> (((nametable_column % 4) / 2) + (((nametable_row % 4) / 2) * 2)) * 2) & 0b11;
        bg_pallete = nes->ppu.vram + 0x3F01 + (pallete_index * 4);
        

        for (u8 bit = 0; bit < 8; bit++)
        {
            u8 pixel = ((*tile_row_hi >> (7 - bit)) & 0x1) | ((*tile_row_low >> (7 - bit)) & 0x1) << 1;
            color = (pixel) ? color = pallete[bg_pallete[pixel - 1]] : pallete[nes->ppu.vram[0x3f00]];
            nes->ppu.framebuffer[(nes->ppu.scanline * 256) + (nametable_column * 8) + bit] = color;
        }
    }
    
    SDL_UpdateTexture(nes->ppu.texture, NULL, nes->ppu.framebuffer, 256 * sizeof(uint32_t));
    SDL_RenderCopy(nes->ppu.renderer, nes->ppu.texture, NULL, NULL);
    //SDL_RenderClear(nes->ppu.renderer);
    SDL_RenderPresent(nes->ppu.renderer);
}

void ppu_write(struct nes *nes, u16 addr, u8 value)
{
    switch(addr & 0x7)
    {
    case 0:
        if (((value & 0x80) & (nes->ppu.registers.PPUSTATUS & 0x80)) == 0x80)
            cpu_interrupt(nes, NMI_VECTOR);
        nes->ppu.registers.PPUCTRL = value;
        break;
    case 1: nes->ppu.registers.PPUMASK = value; break;
    case 3: nes->ppu.registers.OAMADDR = value; break;
    case 5:  nes->ppu.addr_latch = nes->ppu.addr_latch << 8 | value;
    // IMPLEMENT LATER
    // After reading PPUSTATUS to reset the address latch,
    // write the horizontal and vertical scroll offsets here just before turning on the screen:
    break;
    case 6:
        nes->ppu.ppu_addr = (nes->ppu.ppu_addr << 8 | value);
        break;
    case 7:
        nes->ppu.vram[nes->ppu.ppu_addr] = value;
        nes->ppu.ppu_addr = (nes->ppu.registers.PPUCTRL & 0b100) ? nes->ppu.ppu_addr + 32 : nes->ppu.ppu_addr + 1;
        break;
    default:
        printf("Unimplemented PPU WRITE: %02x\n", addr & 0xF);
        nes->cpu.uoc = true;
    }
}

u8 ppu_read(struct nes *nes, u16 addr)
{
    u8 ret_val;
    switch(addr & 0xF)
    {
        case 0: return 0;
        case 1: return 0;
        case 5: return 0;
        case 6: return 0;
        case 3: return 0;
        case 2:
            ret_val = nes->ppu.registers.PPUSTATUS;
            nes->ppu.registers.PPUSTATUS &= ~PPUSTATUS_VBLANK;
            return (ret_val);
        case 7:
            ret_val = nes->ppu.vram[nes->ppu.ppu_addr];
            nes->ppu.ppu_addr = (nes->ppu.registers.PPUCTRL & 0b100) ? nes->ppu.ppu_addr + 32 : nes->ppu.ppu_addr + 1;
            return ret_val;
        default:
            printf("\nUnimplemented PPU READ: %02x\n", addr);
            nes->cpu.uoc = true;
    }
    return 0;
}

void ppu_execute(struct nes *nes)
{
    // The ppu Renders 262 scanlines per frame
    // Each scanline lasts 341 ppu cycles
    nes->ppu.cycles += nes->cpu.cycles;
    if (nes->ppu.cycles > 341)
    {
        nes->ppu.cycles -= 341;
        nes->ppu.scanline++;

                // The PPU just idles during this scanline. Even though accessing PPU memory
        // from the program would be safe here,
        // the VBlank flag isn't set until after this scanline.
        if (nes->ppu.scanline == 240)
            return;

        if (nes->ppu.scanline >= 241 && nes->ppu.scanline < 260)
        {
            nes->ppu.registers.PPUSTATUS |= PPUSTATUS_VBLANK;
            if (nes->ppu.registers.PPUCTRL & PPUCONTROL_NMI)
            {
                cpu_interrupt(nes, NMI_VECTOR);
                nes->ppu.registers.PPUCTRL &= ~PPUCONTROL_NMI;
            }
            return;
        }

        if (nes->ppu.scanline == 260)
        {
            nes->ppu.registers.PPUSTATUS &= ~PPUSTATUS_VBLANK;
            nes->ppu.scanline = 0;
            return;
        }
        if(nes->ppu.scanline == 261) return; //todo
        if(nes->ppu.scanline < 240) ppu_render_pixel(nes);
    }
}