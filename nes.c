#include <stdio.h>
#include <stdlib.h>
#include "nes.h"

#define IMMEDIATE (n->mmu.memory[n->cpu.registers.PC++])
#define RELATIVE (n->mmu.memory[n->cpu.registers.PC++])
#define ZEROPAGE (0x100 | n->mmu.memory[n->cpu.registers.PC++])
#define ABSOLUTE (n->mmu.memory[n->cpu.registers.PC++] | n->mmu.memory[n->cpu.registers.PC++] << 8)

void cpu_instr_jmp(struct NES *n, u16 addr)
{
    n->cpu.registers.PC = addr;
}
void cpu_instr_ldy(struct NES *n, u8 value)
{
    n->cpu.registers.Y = value;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (value & STATUS_NEGATIVE);
    n->cpu.registers.S = (value) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

void cpu_instr_ldx(struct NES *n, u8 value)
{
    n->cpu.registers.X = value;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (value & STATUS_NEGATIVE);
    n->cpu.registers.S = (value) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

void cpu_instr_sta(struct NES *n, u16 addr)
{
    n->mmu.memory[addr] = n->cpu.registers.A;
}
void cpu_instr_lda(struct NES *n, u8 value)
{
    n->cpu.registers.A = value;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (value & STATUS_NEGATIVE);
    n->cpu.registers.S = (value) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}
void cpu_instr_pla(struct NES *n)
{
    n->cpu.registers.A = cpu_pull8(n);
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (n->cpu.registers.A & STATUS_NEGATIVE);
    n->cpu.registers.S = (n->cpu.registers.A) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

void cpu_instr_iny(struct NES *n)
{
    n->cpu.registers.Y++;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (n->cpu.registers.Y & STATUS_NEGATIVE);
    n->cpu.registers.S = (n->cpu.registers.Y) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

void cpu_instr_stx(struct NES *n, u16 addr)
{
    n->mmu.memory[addr] = n->cpu.registers.X;
}
void cpu_instr_branch(struct NES *n, i8 offset, bool cond)
{
    if (cond)
        n->cpu.registers.PC += offset;
}
void cpu_instr_bit(struct NES *n, u16 addr)
{
    n->cpu.registers.S = (n->cpu.registers.S & ~0xC0)| (n->mmu.memory[addr] & 0xC0);
    n->cpu.registers.S =  (n->mmu.memory[addr] & n->cpu.registers.A) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

void cpu_instr_and(struct NES *n, u8 value)
{
    n->cpu.registers.A = n->cpu.registers.A & value;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (n->cpu.registers.A & STATUS_NEGATIVE);
    n->cpu.registers.S = (n->cpu.registers.A) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

void cpu_instr_ora(struct NES *n, u8 value)
{
    n->cpu.registers.A = n->cpu.registers.A | value;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (n->cpu.registers.A & STATUS_NEGATIVE);
    n->cpu.registers.S = (n->cpu.registers.A) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

void cpu_instr_cmp(struct NES *n, u8 value)
{
    u8 result = n->cpu.registers.A - value;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (result & STATUS_NEGATIVE);
    n->cpu.registers.S = (result) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
    n->cpu.registers.S = (n->cpu.registers.A >= value) ? n->cpu.registers.S | STATUS_CARRY : n->cpu.registers.S & ~STATUS_CARRY;
}

void cpu_instr_cpy(struct NES *n, u8 value)
{
    u8 result = n->cpu.registers.Y - value;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (result & STATUS_NEGATIVE);
    n->cpu.registers.S = (result) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
    n->cpu.registers.S = (n->cpu.registers.Y >= value) ? n->cpu.registers.S | STATUS_CARRY : n->cpu.registers.S & ~STATUS_CARRY;
}

void cpu_instr_cpx(struct NES *n, u8 value)
{
    u8 result = n->cpu.registers.X - value;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (result & STATUS_NEGATIVE);
    n->cpu.registers.S = (result) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
    n->cpu.registers.S = (n->cpu.registers.X >= value) ? n->cpu.registers.S | STATUS_CARRY : n->cpu.registers.S & ~STATUS_CARRY;
}

void cpu_instr_eor(struct NES *n, u8 value)
{
    n->cpu.registers.A = n->cpu.registers.A ^ value;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (n->cpu.registers.A & STATUS_NEGATIVE);
    n->cpu.registers.S = (n->cpu.registers.A) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

void cpu_instr_adc(struct NES *n, u8 value)
{
    u16 result = n->cpu.registers.A + value + (n->cpu.registers.S & STATUS_CARRY);

    n->cpu.registers.S = (((n->cpu.registers.A ^ result) & (value ^ result) & 0x80)) ? n->cpu.registers.S | STATUS_OVERFLOW : n->cpu.registers.S & ~STATUS_OVERFLOW;
    n->cpu.registers.A = result & 0xFF;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (n->cpu.registers.A & STATUS_NEGATIVE);
    n->cpu.registers.S = (n->cpu.registers.A) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
    n->cpu.registers.S = (result > 0xFF) ? n->cpu.registers.S | STATUS_CARRY : n->cpu.registers.S & ~STATUS_CARRY;
}

void cpu_instr_sbc(struct NES *n, u8 value)
{
    u16 result = n->cpu.registers.A - value - !n->cpu.registers.S & STATUS_CARRY;

    n->cpu.registers.S = (((n->cpu.registers.A ^ result) & (value ^ result) & 0x80)) ? n->cpu.registers.S | STATUS_OVERFLOW : n->cpu.registers.S & ~STATUS_OVERFLOW;
    n->cpu.registers.S = (n->cpu.registers.A >= value) ? n->cpu.registers.S | STATUS_CARRY : n->cpu.registers.S & ~STATUS_CARRY;
    n->cpu.registers.A = result & 0xFF;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (n->cpu.registers.A & STATUS_NEGATIVE);
    n->cpu.registers.S = (n->cpu.registers.A) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

struct CPU *cpu_init()
{
    struct CPU *cpu = malloc(sizeof(struct CPU));
    return cpu;
}

void cpu_push8(struct NES *n, u8 value)
{
    n->mmu.memory[0x0100 + --n->cpu.registers.SP] = value;
}

u8 cpu_pull8(struct NES *n)
{
    return n->mmu.memory[0x0100 + n->cpu.registers.SP++];
}

void cpu_push16(struct NES *n, u16 value)
{
    // little endian, push low, push high
    // paying attention to decrement orders next time
    n->mmu.memory[0x0100 + --n->cpu.registers.SP] = value >> 8;
    n->mmu.memory[0x0100 + --n->cpu.registers.SP] = value & 0X0FF;
}

u16 cpu_pull16(struct NES *n)
{
    u16 value = n->mmu.memory[0x0100 + n->cpu.registers.SP++]  | n->mmu.memory[0x0100 + n->cpu.registers.SP++] << 8;
    return value;
}

void cpu_tick(struct NES *n)
{
    printf("%04X  %02X  A: %02X X: %02X Y: %02X P: %02X SP: %02X\n", n->cpu.registers.PC, n->mmu.memory[n->cpu.registers.PC], n->cpu.registers.A, n->cpu.registers.X, n->cpu.registers.Y,
        n->cpu.registers.S, n->cpu.registers.SP);
    u8 opcode = n->mmu.memory[n->cpu.registers.PC++];
    switch (opcode)
    {
    case 0x08://    PHP
        cpu_push8(n, n->cpu.registers.S | 0x30);
        break;
    case 0x09://    ORA
        cpu_instr_ora(n, IMMEDIATE);
        break;
    case 0x10://    BPL
        cpu_instr_branch(n, RELATIVE, (n->cpu.registers.S & STATUS_NEGATIVE) != STATUS_NEGATIVE);
        break;
    case 0x18://    CLC
        n->cpu.registers.S &= ~STATUS_CARRY;
        break;
    case 0x20:
        cpu_push16(n, n->cpu.registers.PC + 2);
        cpu_instr_jmp(n, ABSOLUTE);
        break;
    case 0x24://    BIT
        cpu_instr_bit(n, ZEROPAGE);
        break;
    case 0x28://    PLP
        n->cpu.registers.S = (n->cpu.registers.S & 0b00110000) | (cpu_pull8(n) & 0b11001111);
        break;
    case 0x29://    AND
        cpu_instr_and(n, IMMEDIATE);
        break;
    case 0x30://    BMI
        cpu_instr_branch(n, RELATIVE, n->cpu.registers.S & STATUS_NEGATIVE);
        break;
    case 0x38://    SEC
        n->cpu.registers.S |= STATUS_CARRY;
        break;
    case 0x48://    PHA
        cpu_push8(n, n->cpu.registers.A);
        break;
    case 0x4C:
        cpu_instr_jmp(n, ABSOLUTE);
        break;
    case 0x49://    EOR
        cpu_instr_eor(n, IMMEDIATE);
        break;
    case 0x50://    BVC
        cpu_instr_branch(n, RELATIVE, (n->cpu.registers.S & STATUS_OVERFLOW) != STATUS_OVERFLOW);
        break;
    case 0x60://    RTS
        n->cpu.registers.PC = cpu_pull16(n);
        break;
    case 0x68://    PLA
        cpu_instr_pla(n);
        break;
    case 0x69://    ADC
        cpu_instr_adc(n, IMMEDIATE);
        break;
    case 0x70://    BVS
        cpu_instr_branch(n, RELATIVE, n->cpu.registers.S & STATUS_OVERFLOW);
        break;
    case 0x78://    SEI
        n->cpu.registers.S |= STATUS_ID;
        break;
    case 0x85:
        cpu_instr_sta(n, ZEROPAGE);
        break;
    case 0x86:
        cpu_instr_stx(n, ZEROPAGE);
        break;
    case 0x90://    BCC
        cpu_instr_branch(n, RELATIVE, (n->cpu.registers.S & STATUS_CARRY) != STATUS_CARRY);
        break;
    case 0xA0://    LDY
        cpu_instr_ldy(n, IMMEDIATE);
        break;
    case 0xA2://    LDX
        cpu_instr_ldx(n, IMMEDIATE);
        break;
    case 0xA9://    LDA
        cpu_instr_lda(n, IMMEDIATE);
        break;
    case 0xB0://    BCS  
        cpu_instr_branch(n, RELATIVE, n->cpu.registers.S & STATUS_CARRY);
        break;
    case 0xB8://    CLV
        n->cpu.registers.S &= ~STATUS_OVERFLOW;
        break;
    case 0xC0://    CPY
        cpu_instr_cpy(n, IMMEDIATE);
        break;
    case 0xC8://    INY
        cpu_instr_iny(n);
        break;
    case 0xC9://    CMP
        cpu_instr_cmp(n, IMMEDIATE);
        break;
    case 0xD0://    BNE
        cpu_instr_branch(n, RELATIVE, (n->cpu.registers.S & STATUS_ZERO) != STATUS_ZERO);
        break;
    case 0xD8://    CLD
        n->cpu.registers.S &= ~STATUS_DECIMAL;
        break;
    case 0xE0://    CPX
        cpu_instr_cpx(n, IMMEDIATE);
        break;
    case 0xE9://    SBC
        cpu_instr_sbc(n, IMMEDIATE);
        break;
    case 0xEA:
        break;
    case 0xF0://    BEQ
        cpu_instr_branch(n, RELATIVE, n->cpu.registers.S & STATUS_ZERO);
        break;
    case 0xF8://    SED
        n->cpu.registers.S |= STATUS_DECIMAL;
        break;
    default:
        n->uoc = true;
        printf("Unimplemented opcode: %X\n", opcode);
        break;
    }
}

u16 cpu_u16(struct NES *n)
{
    return n->mmu.memory[n->cpu.registers.PC++] | n->mmu.memory[n->cpu.registers.PC++] << 8;
}

void cart_load_rom(struct NES *n, char *romname)
{
    FILE *rom = fopen(romname, "rb");
    fseek(rom, 0, SEEK_END);
    long filesize = ftell(rom);
    fseek(rom, 0, SEEK_SET);

    fread(n->cart.header, sizeof(u8), CART_HEADER_SIZE, rom);
    n->cart.PRG_ROM = malloc(n->cart.header[CART_HEADER_PRG_SIZE] * CART_PRG_ROM_SIZE);
    n->cart.CHR_ROM = malloc(n->cart.header[CART_HEADER_CHR_SIZE] * CART_CHR_ROM_SIZE);
    fread(n->cart.PRG_ROM, sizeof(u8), n->cart.header[CART_HEADER_PRG_SIZE] * CART_PRG_ROM_SIZE, rom);
    fread(n->cart.CHR_ROM, sizeof(u8), n->cart.header[CART_HEADER_CHR_SIZE] * CART_CHR_ROM_SIZE, rom);

    fclose(rom);
}

struct MMU *mmu_init()
{
    struct MMU *mmu = malloc(sizeof(struct MMU));
    return mmu;
}

u8 mmu_read_byte(struct NES *n, u16 addr)
{
    return n->mmu.memory[addr];
}

void mmu_load_rom(struct NES *n)
{
    memcpy(n->mmu.memory + CPU_MMAP_START_ADDR, n->cart.PRG_ROM, CART_PRG_ROM_SIZE);
    // MIRRORED ?
    if (n->cart.header[4] == 1)
        memcpy(n->mmu.memory + CPU_MMAP_START_ADDR + CART_PRG_ROM_SIZE, n->cart.PRG_ROM, CART_PRG_ROM_SIZE);
    // CHR ROM OR CHR RAM ?
    if (n->cart.header[5])
        memcpy(n->mmu.memory, n->cart.CHR_ROM, CART_CHR_ROM_SIZE);
}

struct NES *nes_init()
{
    struct NES *n = malloc(sizeof(struct NES));
}