#include "common.h"
#include "nes.h"


void cart_close(struct nes *nes);
void cart_init(struct nes *nes, char *romname);

void mapper00_init(struct nes *nes);
void mapper_init(struct nes *nes);