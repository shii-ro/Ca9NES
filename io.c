#include "io.h"
#include <stdio.h>

u8 io_joy_read(struct nes *nes, u16 addr)
{
    u8 ret_val = nes->io[addr & 0xFF] & 0x1;
    nes->io[addr & 0xFF] = nes->io[addr & 0xFF] >> 1;
    return 0x40 | ret_val;
}