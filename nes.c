#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "common.h"
#include "nes.h"
#include "cpu.h"
#include "ppu.h"
#include "cart.h"
#include "io.h"

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

            switch (event.type)
            {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                case SDLK_z:
                    nes->keystate |= JOY_BUTTON_A;
                    break;
                case SDLK_x:
                    nes->keystate |= JOY_BUTTON_B;
                    break;
                case SDLK_c:
                    nes->keystate |= JOY_BUTTON_START;
                    break;
                case SDLK_v:
                    nes->keystate |= JOY_BUTTON_SEL;
                    break;
                case SDLK_UP:
                    nes->keystate |= JOY_BUTTON_UP;
                    break;
                case SDLK_LEFT:
                    nes->keystate |= JOY_BUTTON_LEFT;
                    break;
                case SDLK_DOWN:
                    nes->keystate |= JOY_BUTTON_DOWN;
                    break;
                case SDLK_RIGHT:
                    nes->keystate |= JOY_BUTTON_RIGHT;
                    break;
                default:
                    break;
                }break;

            case SDL_KEYUP:
                switch (event.key.keysym.sym)
                {
                case SDLK_z:
                    nes->keystate &= ~JOY_BUTTON_A;
                    break;
                case SDLK_x:
                    nes->keystate &= ~JOY_BUTTON_B;
                    break;
                case SDLK_c:
                    nes->keystate &= ~JOY_BUTTON_START;
                    break;
                case SDLK_v:
                    nes->keystate &= ~JOY_BUTTON_SEL;
                    break;
                case SDLK_UP:
                    nes->keystate &= ~JOY_BUTTON_UP;
                    break;
                case SDLK_LEFT:
                    nes->keystate &= ~JOY_BUTTON_LEFT;
                    break;
                case SDLK_DOWN:
                    nes->keystate &= ~JOY_BUTTON_DOWN;
                    break;
                case SDLK_RIGHT:
                    nes->keystate &= ~JOY_BUTTON_RIGHT;
                    break;
                default:
                    break;
                }break;
            }
        }

        nes->cpu.cycles += cpu_execute(nes);
        ppu_cycles = nes->cpu.cycles * 3;
        for (unsigned i = 0; i < ppu_cycles; i++)
            ppu_tick(nes);
        nes->total_cycles += nes->cpu.cycles;
        nes->cpu.cycles = 0;
    }
}

u8 nes_read8(struct nes *nes, u16 addr)
{
    switch(addr >> 13)
    {
        case 0: return nes->ram[addr & 0x7FF]; // $0000-$1FFF
        case 1: return ppu_read(nes, addr); // 2000-$3FFF
        case 2:                             // $4000-$5FFF
            if(addr == 0x4016) return io_joy_read(nes, 0x4016);
            else if (addr == 0x4017) return io_joy_read(nes, 0x4017);
            printf("NOT IMPLEMENTED IO/APU READ: %04x\n", addr);
            return 0;
        case 3: return nes->mapper.mapper_read(nes, addr); // $6000-$7FFF
        default: return nes->mapper.mapper_read(nes, addr);
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
            else if (addr == 0x4016)
            {
                if (value & 1)
                    nes->io[0x16] = nes->keystate;
            }
            //printf("VALUE : %02x NOT IMPLEMENTED IO/APU WRITE: %04x\n",value,  addr);
            break;
        case 3: nes->mapper.mapper_write(nes, addr, value); break;
        default: nes->mapper.mapper_write(nes, addr, value); break;
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