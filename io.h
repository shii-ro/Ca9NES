#ifndef IO_H
#define IO_H

#include "nes.h"


#define JOY_BUTTON_A 0x1
#define JOY_BUTTON_B 0x2
#define JOY_BUTTON_SEL 0x4
#define JOY_BUTTON_START 0x8
#define JOY_BUTTON_UP 0x10
#define JOY_BUTTON_DOWN 0x20
#define JOY_BUTTON_LEFT 0x40
#define JOY_BUTTON_RIGHT 0x80



u8 io_joy_read(struct nes *nes, u16 addr);

#endif