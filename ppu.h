#include "common.h"
#include "nes.h"

void ppu_init(struct nes *nes);
void ppu_write(struct nes *nes, u16 addr, u8 value);
u8 ppu_read(struct nes *nes, u16 addr);
void ppu_execute(struct nes *nes);
void ppu_tick(struct nes *nes);
