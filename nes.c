#include <stdio.h>
#include <stdlib.h>
#include "nes.h"

#define ADR_IMMEDIATE (n->cpu.registers.PC++)
#define ADR_RELATIVE (n->mmu.memory[n->cpu.registers.PC++])
#define ADR_ZEROPAGE (n->mmu.memory[n->cpu.registers.PC++] | 0x100)
#define ADR_ABSOLUTE (n->mmu.memory[n->cpu.registers.PC++] | n->mmu.memory[n->cpu.registers.PC++] << 8)

u8 mmu_read_byte(struct NES *n, u16 addr)
{
    return n->mmu.memory[addr];
}

void mmu_write_byte(struct NES *n, u16 addr, u8 value)
{
    n->mmu.memory[addr] = value;
}

void cpu_instr_jmp(struct NES *n, u16 addr)
{
    n->cpu.registers.PC = addr;
}

void cpu_instr_ldr(struct NES *n, u8 value, u8 *reg)// Loads register with a value
{
    *reg = value;

    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (value & STATUS_NEGATIVE);
    n->cpu.registers.S = (*reg) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

void cpu_instr_pla(struct NES *n)
{
    n->cpu.registers.A = cpu_pull8(n);
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (n->cpu.registers.A & STATUS_NEGATIVE);
    n->cpu.registers.S = (n->cpu.registers.A) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

void cpu_instr_inr(struct NES *n, u8* reg)
{
    (*reg)++;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (*reg & STATUS_NEGATIVE);
    n->cpu.registers.S = (*reg) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

void cpu_instr_der(struct NES *n, u8 *reg)
{
    (*reg)--;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (*reg & STATUS_NEGATIVE);
    n->cpu.registers.S = (*reg) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

void cpu_instr_branch(struct NES *n, i8 offset, bool cond)
{
    if (cond)
        n->cpu.registers.PC += offset;
}
void cpu_instr_bit(struct NES *n, u8 value)
{
    n->cpu.registers.S = (n->cpu.registers.S & ~0xC0)| (value & 0xC0);
    n->cpu.registers.S =  (value & n->cpu.registers.A) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
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

void cpu_instr_tr(struct NES *n, u8 *dst, u8 *src)
{
    *dst = *src;
    n->cpu.registers.S = (n->cpu.registers.S & ~STATUS_NEGATIVE) | (*dst & STATUS_NEGATIVE);
    n->cpu.registers.S = (*dst) ? n->cpu.registers.S & ~STATUS_ZERO : n->cpu.registers.S | STATUS_ZERO;
}

struct CPU *cpu_init()
{
    struct CPU *cpu = malloc(sizeof(struct CPU));
    return cpu;
}

void cpu_push8(struct NES *n, u8 value)
{
    if(value == 0x4E)
        n->uoc = true;
    n->mmu.memory[0x0100 | n->cpu.registers.SP--] = value;
}

u8 cpu_pull8(struct NES *n)
{
    return n->mmu.memory[0x0100 + ++n->cpu.registers.SP];
}

void cpu_push16(struct NES *n, u16 value)
{
    // little endian, push low, push high
    // paying attention to decrement orders next time
    n->mmu.memory[0x0100 | n->cpu.registers.SP--] = value >> 8;
    n->mmu.memory[0x0100 | n->cpu.registers.SP--] = value & 0X0FF;
}

u16 cpu_pull16(struct NES *n)
{
    u8 low = n->mmu.memory[0x0100 | ++n->cpu.registers.SP];
    u8 hi = n->mmu.memory[0x0100 | ++n->cpu.registers.SP];
    u16 value = hi << 8 | low;
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
        cpu_instr_ora(n, mmu_read_byte(n, ADR_IMMEDIATE));
        break;
    case 0x10://    BPL
        cpu_instr_branch(n, ADR_RELATIVE, (n->cpu.registers.S & STATUS_NEGATIVE) != STATUS_NEGATIVE);
        break;
    case 0x18://    CLC
        n->cpu.registers.S &= ~STATUS_CARRY;
        break;
    case 0x20:
        cpu_push16(n, n->cpu.registers.PC + 1);
        cpu_instr_jmp(n, ADR_ABSOLUTE);
        break;
    case 0x24://    BIT
        cpu_instr_bit(n, mmu_read_byte(n, ADR_ZEROPAGE));
        break;
    case 0x28://    PLP
        n->cpu.registers.S = (n->cpu.registers.S & 0b00110000) | (cpu_pull8(n) & 0b11001111);
        break;
    case 0x29://    AND
        cpu_instr_and(n, mmu_read_byte(n, ADR_IMMEDIATE));
        break;
    case 0x30://    BMI
        cpu_instr_branch(n, ADR_RELATIVE, n->cpu.registers.S & STATUS_NEGATIVE);
        break;
    case 0x38://    SEC
        n->cpu.registers.S |= STATUS_CARRY;
        break;
    case 0x48://    PHA
        cpu_push8(n, n->cpu.registers.A);
        break;
    case 0x4C:
        cpu_instr_jmp(n, ADR_ABSOLUTE);
        break;
    case 0x49://    EOR
        cpu_instr_eor(n, mmu_read_byte(n, ADR_IMMEDIATE));
        break;
    case 0x50://    BVC
        cpu_instr_branch(n, ADR_RELATIVE, (n->cpu.registers.S & STATUS_OVERFLOW) != STATUS_OVERFLOW);
        break;
    case 0x60://    RTS
        n->cpu.registers.PC = cpu_pull16(n) + 1;
        break;
    case 0x68://    PLA
        cpu_instr_pla(n);
        break;
    case 0x69://    ADC
        cpu_instr_adc(n, mmu_read_byte(n, ADR_IMMEDIATE));
        break;
    case 0x70://    BVS
        cpu_instr_branch(n, ADR_RELATIVE, n->cpu.registers.S & STATUS_OVERFLOW);
        break;
    case 0x78://    SEI
        n->cpu.registers.S |= STATUS_ID;
        break;
    //case 0x84://    STY
        //mmu_write_byte(n, ZEROPAGE, n->cpu.registers.A);
       // break;
    case 0x85://    STA
        mmu_write_byte(n, ADR_ZEROPAGE, n->cpu.registers.A);
        break;
    case 0x86://    STX
        mmu_write_byte(n, ADR_ZEROPAGE, n->cpu.registers.X);
        break;
    case 0x88://    DEY
        cpu_instr_der(n, &n->cpu.registers.Y);
        break;
    case 0x8A://    TXA
        cpu_instr_tr(n, &n->cpu.registers.A, &n->cpu.registers.X);
        break;
    case 0x8E://    STX
        mmu_write_byte(n, ADR_ABSOLUTE, n->cpu.registers.X);
        break;
    case 0x90://    BCC
        cpu_instr_branch(n, ADR_RELATIVE, (n->cpu.registers.S & STATUS_CARRY) != STATUS_CARRY);
        break;
    case 0x98://    TYA
        cpu_instr_tr(n, &n->cpu.registers.A, &n->cpu.registers.Y);
        break;
    case 0x9A://    TXS
        n->cpu.registers.SP = n->cpu.registers.X;
        break;
    case 0xA0://    LDY
        cpu_instr_ldr(n, mmu_read_byte(n, ADR_IMMEDIATE), &n->cpu.registers.Y);
        break;
    case 0xA2://    LDX
        cpu_instr_ldr(n, mmu_read_byte(n, ADR_IMMEDIATE), &n->cpu.registers.X);
        break;
    case 0xA8://    TAY
        cpu_instr_tr(n, &n->cpu.registers.Y, &n->cpu.registers.A);
        break;
    case 0xA9://    LDA
        cpu_instr_ldr(n, mmu_read_byte(n, ADR_IMMEDIATE), &n->cpu.registers.A);
        break;
    case 0xAA://    TAX
        cpu_instr_tr(n, &n->cpu.registers.X, &n->cpu.registers.A);
        break;
    case 0xAD://    LDA ABSOLUTE
        cpu_instr_ldr(n, mmu_read_byte(n, ADR_ABSOLUTE), &n->cpu.registers.A);
        break;
    case 0xAE://    LDX ABSOLUTE
        cpu_instr_ldr(n, mmu_read_byte(n, ADR_ABSOLUTE), &n->cpu.registers.X);
        break;
    case 0xB0://    BCS  
        cpu_instr_branch(n, ADR_RELATIVE, n->cpu.registers.S & STATUS_CARRY);
        break;
    case 0xB8://    CLV
        n->cpu.registers.S &= ~STATUS_OVERFLOW;
        break;
    case 0xBA://    TSX
        cpu_instr_tr(n, &n->cpu.registers.X, &n->cpu.registers.SP);
        break;
    case 0xC0://    CPY
        cpu_instr_cpy(n, mmu_read_byte(n, ADR_IMMEDIATE));
        break;
    case 0xC8://    INY
        cpu_instr_inr(n, &n->cpu.registers.Y);
        break;
    case 0xC9://    CMP
        cpu_instr_cmp(n, mmu_read_byte(n, ADR_IMMEDIATE));
        break;
    case 0xCA://    DEX
        cpu_instr_der(n, &n->cpu.registers.X);
        break;
    case 0xD0://    BNE
        cpu_instr_branch(n, ADR_RELATIVE, (n->cpu.registers.S & STATUS_ZERO) != STATUS_ZERO);
        break;
    case 0xD8://    CLD
        n->cpu.registers.S &= ~STATUS_DECIMAL;
        break;
    case 0xE0://    CPX
        cpu_instr_cpx(n, mmu_read_byte(n, ADR_IMMEDIATE));
        break;
    case 0xE8://    INX
        cpu_instr_inr(n, &n->cpu.registers.X);
        break;
    case 0xE9://    SBC
        cpu_instr_adc(n, ~mmu_read_byte(n, ADR_IMMEDIATE));
        break;
    case 0xEA:
        break;
    case 0xF0://    BEQ
        cpu_instr_branch(n, ADR_RELATIVE, n->cpu.registers.S & STATUS_ZERO);
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
