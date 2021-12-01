#ifndef CPU_H
#define CPU_H


#include "common.h"
#include "nes.h"

void cpu_init(struct nes *nes);
void cpu_reset(struct nes *nes);
u8 cpu_execute(struct nes *nes);
void cpu_interrupt(struct nes *nes, u16 vector);
#endif


