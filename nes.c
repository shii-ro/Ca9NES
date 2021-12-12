#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "nes.h"
#include "cpu.h"
#include "ppu.h"
#include "cart.h"

void nes_run(struct nes *nes)
{
    unsigned ppu_cycles;
    SDL_Event event;
    bool quit;
    while (!nes->cpu.uoc && !quit)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                quit = true;
        }
        nes->cpu.cycles += cpu_execute(nes);
        ppu_cycles = nes->cpu.cycles * 3;
        for(unsigned i = 0; i < ppu_cycles; i++) ppu_tick(nes);
        // ppu_execute(nes);
        // ppu_execute(nes);
        // ppu_execute(nes);
        nes->total_cycles += nes->cpu.cycles;
        nes->cpu.cycles = 0;
    }
}

u8 nes_read8(struct nes *nes, u16 addr)
{
    switch(addr >> 13)
    {
        case 0: return nes->ram[addr & 0x7FF]; // $0000-$07FF
        case 1: return ppu_read(nes, addr);
        case 2:
            //printf("NOT IMPLEMENTED IO/APU READ: %04x\n", addr);
            return 0;
        case 3:
           // printf("NOT IMPLEMENTED READ %04x\n", addr);
            nes->cpu.uoc = true;
            return 0;
        default: return nes->cart.mapper.mapper_read(&nes->cart.mapper, addr);
    };
}

u16 nes_read16(struct nes *nes, u16 addr)
{
    return nes_read8(nes, addr) | nes_read8(nes, addr + 1) << 8;
}

void nes_write8(struct nes *nes, u16 addr, u8 value)
{
    switch(addr >> 13)
    {
        case 0: nes->ram[addr & 0x7FF] = value; break; // $0000-$07FF;
        case 1: ppu_write(nes, addr, value); break;
        case 2:
            if (addr == 0x4014)
            {
                u16 baseaddr = nes->ppu.registers.OAMADDR << 8 | 0x00;
                for (int i = 0; i < 256; i++)
                {
                    nes->ppu.oam.oam_bytes[i] = nes_read8(nes, baseaddr + 0x200 + i);
                }
                return;
            }
            //printf("NOT IMPLEMENTED IO/APU WRITE: %04x\n", addr);
            break;
        case 3:
            printf("NOT IMPLEMENTED CART WRITE %04x\n", addr);
            nes->cpu.uoc = true;
            break;
        default: nes->cart.mapper.mapper_read(&nes->cart.mapper, addr); break;
    };
}

void nes_write16(struct nes *nes, u16 addr, u16 value)
{
    nes_write8(nes, addr, value & 0xFF);
    nes_write8(nes, addr + 1, value >> 8);
}

void nes_init(struct nes *nes, char *romname)
{
    cpu_init(nes);
    cart_init(nes, romname);
    ppu_init(nes);
    cpu_reset(nes);
}

void nes_close(struct nes *nes)
{
    cart_close(nes);
    free(nes);
}