#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nes.h"

int main(int argc, char **argv)
{
    if (argc != 2 || argv[1] == NULL)
    {
        printf("Usage: ./nes romname");
        return 1;
    }
    struct NES *n = nes_init();

    cart_load_rom(n, argv[1]);
    mmu_load_rom(n);

    n->cpu.registers.PC = 0xC000;
    n->cpu.registers.S = 0x24;
    n->cpu.registers.SP = 0xFD;
    while (!n->uoc)
    {
        cpu_tick(n);
    }

    free(n);
    n = NULL;
    return 0;
}