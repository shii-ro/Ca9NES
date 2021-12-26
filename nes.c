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
    while (!nes->cpu.uoc && !nes->quit)
    {
        nes->cpu.cycles += cpu_execute(nes);
        ppu_cycles = nes->cpu.cycles * 3;
        for (unsigned i = 0; i < ppu_cycles; i++)
            ppu_tick(nes);
        nes->total_cycles += nes->cpu.cycles;
        nes->cpu.cycles = 0;
        cpu_process_interrupts(nes);
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
            // printf("NOT IMPLEMENTED IO/APU READ: %04x\n", addr);
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

                u16 baseaddr = (nes->ppu.registers.OAMADDR << 8) | 0x00;
                
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

            break;
        case 3: nes->mapper.mapper_write(nes, addr, value); break;
        default: nes->mapper.mapper_write(nes, addr, value); break;
    };
}

void nes_read_prg_ram(struct nes *nes)
{
    char *save_name = malloc(sizeof(u8) * 30);
    sprintf(save_name, "save_data/%s.sav", nes->romname);
    FILE *sav_data = fopen(save_name, "rb+");
    if (sav_data == NULL) //if file does not exist, create it
    {
        sav_data = fopen(save_name, "wb");
    }
    sav_data = fopen(save_name, "rb+");
    fread(nes->mapper.prg_ram, sizeof(u8), 0x2000, sav_data);
    fclose(sav_data);
    free(save_name);
}

void nes_write_prg_ram(struct nes *nes)
{
    char *save_name = malloc(sizeof(u8) * 30);
    sprintf(save_name, "save_data/%s.sav", nes->romname);
    FILE *save = fopen(save_name, "wb");
    fwrite(nes->mapper.prg_ram, sizeof(u8), 0x2000, save);
    fclose(save);
    free(save_name);
}

void nes_write_save_state(struct nes *nes)
{
    char *state_name = malloc(sizeof(u8) * 30);
    sprintf(state_name, "save_data/%s.state", nes->romname);
    FILE *state_data = fopen(state_name, "wb+");
    fwrite(nes, sizeof(u8), sizeof(struct nes), state_data);
    fclose(state_data);
    free(state_name);
}

void nes_load_save_state(struct nes *nes)
{
    char *state_name = malloc(sizeof(u8) * 30);
    sprintf(state_name, "save_data/%s.state", nes->romname);
    FILE *state_data = fopen(state_name, "rb+");
    if (state_data == NULL) //if file does not exist, create it
    {
        state_data = fopen(state_name, "wb+");
    }
    state_data = fopen(state_name, "rb+");
    fread(nes, sizeof(u8), sizeof(struct nes), state_data);
    fclose(state_data);
    free(state_name);
    SDL_RenderClear(nes->ppu.renderer);
    nes->test_toogle = true;
}

void nes_write16(struct nes *nes, u16 addr, u16 value)
{
    nes_write8(nes, addr, value & 0xFF);
    nes_write8(nes, addr + 1, value >> 8);
}

void nes_init(struct nes *nes, char *romname)
{
    nes->romname = romname;
    cpu_init(nes);
    cart_init(nes, romname);
    ppu_init(nes);
    nes_reset(nes);
}

void nes_reset(struct nes *nes)
{
    nes->cpu.intr_pending[0] = true;
    cpu_interrupt(nes, RESET_VECTOR, nes->cpu.intr_pending[0]);
}

void nes_close(struct nes *nes)
{
    cart_close(nes);
    free(nes);
}