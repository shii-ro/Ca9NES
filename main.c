#include <stdio.h>
#include <stdlib.h>
#include "nes.h"
#include "cpu.h"
#include "cart.h"
#include "ppu.h"


int main(int argc, char *argv[])
{
    if (argc != 2 || argv[1] == NULL)
    {
        printf("Usage: ./desunes romname\n");
        return 1;
    }

    struct nes *nes = malloc(sizeof(struct nes));

    nes_init(nes, argv[1]);
    nes_run(nes);
    nes_close(nes);
}