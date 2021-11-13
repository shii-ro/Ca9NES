#define NES_MEM_SIZE 0x10000
#define NES_INTERNAL_RAM_SIZE 0x0800
#define NES_RAM_MIRROR_SIZE 0x1800
#define NES_PPU_REGISTERS_SIZE 0x08
#define NES_PPU_MIRROR_SIZE 0x1FF8
#define NES_IO_REGISTERS_SIZE 0x0187
#define NES_DISABLED_IO_SIZE 0x08
#define NES_CARTRIDGE_SIZE 0xBFE0
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int8_t i8;


/////////////////////////////////////////////////////

struct CPU
{
    struct
    {
        union
        {
            u16 PC; // Program Counter
            struct
            {
                u8 PCL, PCH;
            };
        };
        u8 X, Y; // Index Registers
        u8 S;    // Status Register
        u8 SP;   // Stack Pointer
        u8 A;   // Accumulator
    } registers;
};
#define STATUS_NEGATIVE 0b10000000
#define STATUS_OVERFLOW 0b01000000
#define STATUS_DECIMAL  0b00001000
#define STATUS_ID       0b00000100
#define STATUS_ZERO     0b00000010
#define STATUS_CARRY    0b00000001


//////////////////////////////////////////////////////////////////

#define CART_HEADER_PRG_SIZE 4
#define CART_HEADER_CHR_SIZE 5
#define CART_HEADER_SIZE 0x10
#define CART_PRG_ROM_SIZE 0x4000
#define CART_CHR_ROM_SIZE 0x2000

struct Cart
{
    u8 header[CART_HEADER_SIZE];
    u8 *PRG_ROM;
    u8 *CHR_ROM;
};

///////////////////////////////////////////////////////////////

#define PPU_MMAP_START_ADDR 0x0000
#define CPU_MMAP_START_ADDR 0x8000

struct MMU
{
    u8 memory[NES_MEM_SIZE];
};


struct NES
{
    struct CPU cpu;
    struct MMU mmu;
    struct Cart cart;
    bool uoc;
};

struct NES *nes_init();
void cart_load_rom(struct NES *n, char *romname);

void mmu_load_rom(struct NES *n);
u8 mmu_read_byte(struct NES *n, u16 addr);
void mmu_write_byte(struct NES *n, u16 addr, u8 value);

void cpu_tick(struct NES *n);
u16 cpu_u16(struct NES *n);
u8 cpu_pull8(struct NES *n);
u16 cpu_pull16(struct NES *n);
void cpu_push8(struct NES *n, u8 value);
void cpu_push16(struct NES *n, u16 value);
void cpu_instr_jmp(struct NES *n, u16 addr);
void cpu_instr_ldr(struct NES *n, u8 value, u8 *reg);
void cpu_instr_inr(struct NES *n, u8* reg);
void cpu_instr_der(struct NES *n, u8 *reg);
