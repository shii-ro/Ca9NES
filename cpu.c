#include "common.h"
#include "nes.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

// REGISTERS
#define PC (nes->cpu.registers.pc)
#define A (nes->cpu.registers.a)
#define X (nes->cpu.registers.x)
#define S (nes->cpu.registers.s)
#define Y (nes->cpu.registers.y)
#define S (nes->cpu.registers.s)
#define SP (nes->cpu.registers.sp)

// UTILS
#define READ8(addr) (nes_read8(nes, addr))
#define READ16(addr) (READ8(addr) | READ8(addr + 1) << 8)
#define WRITE8(addr, value) (nes_write8(nes, addr, value))

static inline void push8(struct nes *nes, u8 value)
{
    WRITE8(SP-- | 0x100, value);
}

static inline void push16(struct nes *nes, u16 value)
{
    push8(nes, (value >> 8) & 0xFF);
    push8(nes, value & 0xFF);
}

static inline u8 pull8(struct nes *nes)
{
    return READ8(++SP | 0x100);
}
static inline u16 pull16(struct nes *nes)
{
    u8 low = pull8(nes);
    u8 hi = pull8(nes);
    return hi << 8 | low;
}

#define PUSH8(value) push8(nes, value)
#define PUSH16(value) push16(nes, value)
#define PULL8() pull8(nes)
#define PULL16() pull16(nes)

#define PAGECROSS(a, b)                  \
    do                                   \
    {                                    \
        if ((((a) ^ (b)) & 0xFF00) != 0) \
            nes->cpu.cycles++;           \
    } while (0);

// ADDRESSING MODES
#define ADRS_ACC()
#define ADRS_IMP()
#define ADRS_ABS()                         \
    do                                     \
    {                                      \
        u8 low = READ8(PC++);              \
        op_addr = (low | READ8(PC++) << 8); \
    } while (0);
#define ADRS_REL() (op_addr = PC++)
#define ADRS_ZP()              \
    do                         \
    {                          \
        op_addr = READ8(PC++); \
    } while (0);

#define ADRS_ZP_Y()                       \
    do                                    \
    {                                     \
        op_addr = ((READ8(PC++)) + Y) & 0xFF;      \
    } while (0);

#define ADRS_ZP_X()                       \
    do                                    \
    {                                     \
        op_addr = ((READ8(PC++)) + X) & 0xFF;      \
    } while (0);

#define ADRS_IMM()                 \
    do                             \
    {                              \
        op_addr = PC++;            \
    } while (0);

#define ADRS_ABS_X()                            \
    do                                          \
    {                                           \
        u8 low = READ8(PC++);                   \
        op_addr = (low | READ8(PC++) << 8) + X; \
        PAGECROSS(op_addr, op_addr - X);        \
    } while (0);

#define __ADRS_ABS_X()                          \
    do                                          \
    {                                           \
        u8 low = READ8(PC++);                   \
        op_addr = (low | READ8(PC++) << 8) + X; \
    } while (0);

#define ADRS_ABS_Y()                            \
    do                                          \
    {                                           \
        u8 low = READ8(PC++);                   \
        op_addr = (low | READ8(PC++) << 8) + Y; \
        PAGECROSS(op_addr, (op_addr - Y));      \
    } while (0);

#define __ADRS_ABS_Y()                           \
    do                                           \
    {                                            \
        u8 low = READ8(PC++);                    \
        op_addr = (low | READ8(PC++) << 8) + Y; \
    } while (0);

#define ADRS_IND()                                                                          \
    do                                                                                      \
    {                                                                                       \
        u8 low = READ8(PC++);                                                               \
        op_addr = (low | READ8(PC++) << 8);                                                 \
        op_addr = READ8(op_addr) | READ8((op_addr & 0xFF00) | ((op_addr + 1) & 0xFF)) << 8; \
    } while (0);

#define ADRS_INDX()                                                  \
    do                                                               \
    {                                                                \
        op_addr = (READ8(PC++) + X) & 0xFF;                          \
        op_addr = READ8((op_addr + 1) & 0xFF) << 8 | READ8(op_addr); \
    } while (0);
#define ADRS_INDY()                                                          \
    do                                                                       \
    {                                                                        \
        op_addr = READ8(PC++);                                               \
        op_addr = (READ8(op_addr) | (READ8((op_addr + 1) & 0xFF)) << 8) + Y; \
        PAGECROSS(op_addr, op_addr - Y);                                     \
    } while (0);
#define __ADRS_INDY()                                                        \
    do                                                                       \
    {                                                                        \
        op_addr = READ8(PC++);                                               \
        op_addr = (READ8(op_addr) | (READ8((op_addr + 1) & 0xFF)) << 8) + Y; \
    } while (0);
//FLAGS
#define CHANGE_FLAG(flag, cond) (S = (cond) ? (S | flag) : (S & ~ flag))
#define CHANGE_NZ(value)                                        \
    do                                                          \
    {                                                           \
        S = (S & ~STATUS_NEGATIVE) | (value & STATUS_NEGATIVE); \
        S = (value) ? (S & ~STATUS_ZERO) : (S | STATUS_ZERO);   \
    } while (0);

// INSTRUCTIONS
#define NOP()
#define LD(reg)                    \
    do                             \
    {                              \
        op_value = READ8(op_addr); \
        reg = op_value;            \
        CHANGE_NZ(reg);            \
    } while (0);

#define ST(reg)do { WRITE8(op_addr, reg);} while (0);

#define BRANCH(cond)                          \
    do                                        \
    {                                         \
        op_value = READ8(op_addr);            \
        if (cond)                             \
        {                                     \
            nes->cpu.cycles++;                \
            PAGECROSS(PC + (i8)op_value, PC); \
            PC += (i8)op_value;               \
        }                                     \
    } while (0);
// BITWISE INSTRUCTIONS
#define ADC()                                                                      \
    do                                                                             \
    {                                                                              \
        op_value = READ8(op_addr);                                                 \
        u16 result = A + op_value + (S & STATUS_CARRY);                            \
        CHANGE_FLAG(STATUS_OVERFLOW, ((A ^ result) & (op_value ^ result) & 0x80)); \
        A = result & 0xFF;                                                         \
        CHANGE_NZ(A);                                                              \
        CHANGE_FLAG(STATUS_CARRY, (result > 0xFF));                                \
    } while (0);
#define SBC()                                                                      \
    do                                                                             \
    {                                                                              \
        op_value = READ8(op_addr);                                                 \
        op_value = (u8)~op_value;                                                  \
        u16 result = A + op_value + (S & STATUS_CARRY);                            \
        CHANGE_FLAG(STATUS_OVERFLOW, ((A ^ result) & (op_value ^ result) & 0x80)); \
        A = result & 0xFF;                                                         \
        CHANGE_NZ(A);                                                              \
        CHANGE_FLAG(STATUS_CARRY, (result > 0xFF));                                \
    } while (0);
#define AND()                      \
    do                             \
    {                              \
        op_value = READ8(op_addr); \
        A = op_value & A;          \
        CHANGE_NZ(A)               \
    } while (0);
#define ORA()                      \
    do                             \
    {                              \
        op_value = READ8(op_addr); \
        A = A | op_value;          \
        CHANGE_NZ(A)               \
    } while (0);
#define EOR()                      \
    do                             \
    {                              \
        op_value = READ8(op_addr); \
        A = A ^ op_value;          \
        CHANGE_NZ(A)               \
    } while (0);
#define CP(reg)                                     \
    do                                              \
    {                                               \
        op_value = READ8(op_addr);                  \
        u8 result = reg - op_value;                 \
        CHANGE_NZ(result);                          \
        CHANGE_FLAG(STATUS_CARRY, (reg >= result)); \
    } while (0);
#define BIT()                                            \
    do                                                   \
    {                                                    \
        op_value = READ8(op_addr);                       \
        CHANGE_FLAG(STATUS_ZERO, !(op_value & A));       \
        CHANGE_FLAG(STATUS_NEGATIVE, (op_value & 0x80)); \
        CHANGE_FLAG(STATUS_OVERFLOW, (op_value & 0x64)); \
    } while (0);
#define LSR()                                        \
    do                                               \
    {                                                \
        op_value = READ8(op_addr);                   \
        CHANGE_FLAG(STATUS_CARRY, (op_value & 0x1)); \
        op_value = (op_value >> 1);                  \
        CHANGE_NZ(op_value);                         \
        WRITE8(op_addr, op_value);                   \
    } while (0);

#define LSRA()                                \
    do                                        \
    {                                         \
        CHANGE_FLAG(STATUS_CARRY, (A & 0x1)); \
        A = (A >> 1);                         \
        CHANGE_NZ(A);                     \
    } while (0);

#define ASL()                                       \
    do                                              \
    {                                               \
        op_value = READ8(op_addr);                  \
        CHANGE_FLAG(STATUS_CARRY, (op_value >> 7)); \
        op_value = op_value << 1;                   \
        CHANGE_NZ(op_value);                        \
        WRITE8(op_addr, op_value);                  \
    } while (0);

#define ASLA()                               \
    do                                       \
    {                                        \
        CHANGE_FLAG(STATUS_CARRY, (A >> 7)); \
        A = A << 1;                          \
        CHANGE_NZ(A);                    \
    } while (0);

#define ROR()                                                 \
    do                                                        \
    {                                                         \
        op_value = READ8(op_addr);                            \
        u8 n_carry = op_value & 0x1;                          \
        op_value = (op_value >> 1) | (S & STATUS_CARRY) << 7; \
        CHANGE_FLAG(STATUS_CARRY, n_carry);                   \
        CHANGE_NZ(op_value);                                  \
        WRITE8(op_addr, op_value);                            \
    } while (0);

#define RORA()                                  \
    do                                          \
    {                                           \
        u8 n_carry = A & 0x1;                   \
        A = (A >> 1) | (S & STATUS_CARRY) << 7; \
        CHANGE_FLAG(STATUS_CARRY, n_carry);     \
        CHANGE_NZ(A);                           \
    } while (0);

#define ROL()                                            \
    do                                                   \
    {                                                    \
        op_value = READ8(op_addr);                       \
        u8 n_carry = (op_value & 0x80);                  \
        op_value = (op_value << 1) | (S & STATUS_CARRY); \
        CHANGE_FLAG(STATUS_CARRY, n_carry);              \
        CHANGE_NZ(op_value);                             \
        WRITE8(op_addr, op_value);                       \
    } while (0);

#define ROLA()                                 \
    do                                         \
    {                                          \
        u8 n_carry = (A & 0x80);               \
        A = (A << 1) | (S & STATUS_CARRY); \
        CHANGE_FLAG(STATUS_CARRY, n_carry);    \
        CHANGE_NZ(A);                          \
    } while (0);

#define INC()                      \
    do                             \
    {                              \
        op_value = READ8(op_addr); \
        op_value++;                \
        CHANGE_NZ(op_value);       \
        WRITE8(op_addr, op_value); \
    } while (0);

#define DEC()                      \
    do                             \
    {                              \
        op_value = READ8(op_addr); \
        op_value--;                \
        CHANGE_NZ(op_value);       \
        WRITE8(op_addr, op_value); \
    } while (0);
// STACK INSTRUCTIONS
#define PHA() PUSH8(A)
#define PLA() do {A = PULL8(); CHANGE_NZ(A);} while(0);
#define PHP() PUSH8(S | 0b00110000)
#define PLP() (S = (S & 0b00110000) |  (PULL8() & 0b11001111))
#define TSX() do {X = SP; CHANGE_NZ(X)} while(0);
#define TXS() (SP = X)


// FLOW CONTROL
#define JSR()           \
    do                  \
    {                   \
        PUSH16(PC - 1); \
        PC = op_addr;   \
    } while (0);

#define RTS()              \
    do                     \
    {                      \
        PC = PULL16() + 1; \
    } while (0);
#define RTI()                                          \
    do                                                 \
    {                                                  \
        S = (S & 0b00110000) | (PULL8() & 0b11001111); \
        PC = PULL16();                                 \
    } while (0);

// REGISTER INSTURCTIONS
#define IN(reg) do {reg++; CHANGE_NZ(reg);} while(0);
#define DE(reg) do {reg--; CHANGE_NZ(reg);} while(0);
#define T(a, b) do {b = a; CHANGE_NZ(b);} while(0);

void cpu_interrupt(struct nes *nes, u16 vector)
{
    PUSH16(PC);
    PUSH8(S);
    nes->cpu.registers.s |= STATUS_ID;
    nes->cpu.registers.pc = READ16(vector);
}

u8 cpu_execute(struct nes *nes)
{
    u16 op_addr;
    u8 op_value;
    u8 opcode;

    // printf("%04X\t", PC);
    opcode = READ8(PC++);
    // printf("%02X A: %02X X: %02X Y: %02X S: %02X SP: %02X CYC: %d \n",
    //        opcode,
    //        A,
    //        X,
    //        Y,
    //        S,
    //        SP,
    //        nes->total_cycles);
    //printf("%02x %02x %02x %02x %02x %02x\n", nes->ram[SP], nes->ram[SP + 1], nes->ram[SP + 2], nes->ram[SP + 3],  nes->ram[SP+ 4],  nes->ram[SP + 5]);
    switch (opcode)
    {
        case 0xEA: ADRS_IMP(); NOP(); return 2; // NOP IMP
        case 0x20: ADRS_ABS(); JSR(); return 6; // JSR ABSOLUTE
        case 0x6C: ADRS_IND(); PC = op_addr; return 5;// JMP INDIRECT
        case 0x4C: ADRS_ABS(); PC = op_addr; return 3; // JMP ABS
        case 0x60: ADRS_IMP(); RTS(); return 6; // RTS IMP
        // LOAD INSTRUCTIONS
        case 0xA9: ADRS_IMM(); LD(A); return 2; // LDA IMMEDIATE
        case 0xAD: ADRS_ABS(); LD(A); return 4; // LDA ABSOLUTE
        case 0xB9: ADRS_ABS_Y(); LD(A); return 4; // LDA INDY
        case 0xA5: ADRS_ZP(); LD(A); return 3; // LDA ZEROPAGE
        case 0xB1: ADRS_INDY(); LD(A); return 5; // lDA INDIRECT Y
        case 0xA1: ADRS_INDX(); LD(A); return 6; // LDA A INDIRECT
        case 0xB5: ADRS_ZP_X(); LD(A); return 4;  // LDA ZP X
        case 0xBD: ADRS_ABS_X(); LD(A); return 4; // LDA ABS X

        case 0xA2: ADRS_IMM(); LD(X); return 2; // LDX IMMEDIATE
        case 0xA6: ADRS_ZP(); LD(X); return 3; // LDX ZERO PAGE
        case 0xAE: ADRS_ABS(); LD(X); return 4; // LDX ABSOLUTE
        case 0xB6: ADRS_ZP_Y(); LD(X); return 4; // LDX ZP Y
        case 0xBE: ADRS_ABS_Y(); LD(X); return 4; // LDX ABS Y

        case 0xA0: ADRS_IMM(); LD(Y); return 2; // LDY IMMEDIATE
        case 0xA4: ADRS_ZP(); LD(Y); return 3; // LDY ZERO PAGE
        case 0xAC: ADRS_ABS(); LD(Y); return 4; // LDY ABSOLUTE
        case 0xB4: ADRS_ZP_X(); LD(Y); return 4; // LDY ZERO PAGE
        case 0xBC: ADRS_ABS_X(); LD(Y); return 4; // LDY ABSOLUTE X
        // STORE INSTRUCTIONS
        case 0x84: ADRS_ZP(); ST(Y); return 3; // STY ZP
        case 0x8C: ADRS_ABS(); ST(Y); return 4; // STY ABSOLUTE
        case 0x94: ADRS_ZP_X(); ST(Y); return 4; // STY ZP X

        case 0x86: ADRS_ZP(); ST(X); return 3; // STX ZP
        case 0x8E: ADRS_ABS(); ST(X); return 4; // STX ABSOLUTE
        case 0x96: ADRS_ZP_Y(); ST(X); return 4; // STX ZP Y

        case 0x81: ADRS_INDX(); ST(A); return 6; // STA X INDIRECT
        case 0x85: ADRS_ZP(); ST(A); return 3; // STA ZP
        case 0x8D: ADRS_ABS(); ST(A); return 4; // STA ABSOLUTE
        case 0x91: __ADRS_INDY(); ST(A); return 6; // STA INDIRECT Y
        case 0x99: __ADRS_ABS_Y(); ST(A); return 5; // STA ABSOLUTE INDEXED Y
        case 0x95: ADRS_ZP_X(); ST(A); return 4; // STA ZP X    
        case 0x9D: __ADRS_ABS_X(); ST(A); return 5; // STA ABS X
        // FLAG INSTRUCTIONS
        case 0x18: ADRS_IMP(); CHANGE_FLAG(STATUS_CARRY, false); return 2; // CLC
        case 0x38: ADRS_IMP(); CHANGE_FLAG(STATUS_CARRY, true); return 2; // SEC IMP
        case 0x58: ADRS_IMP(); CHANGE_FLAG(STATUS_ID, false); return 2; // CLI
        case 0x78: ADRS_IMP(); CHANGE_FLAG(STATUS_ID, true); return 2; // SEI
        case 0xB8: ADRS_IMP(); CHANGE_FLAG(STATUS_OVERFLOW, false); return 2; // CLV
        case 0xD8: ADRS_IMP(); CHANGE_FLAG(STATUS_DECIMAL, false); return 2; // CLD
        case 0xF8: ADRS_IMP(); CHANGE_FLAG(STATUS_DECIMAL, true); return 2; // SED
        // BRANCHES
        case 0x10: ADRS_REL(); BRANCH(!(S & STATUS_NEGATIVE)); return 2; // BPL
        case 0x30: ADRS_REL(); BRANCH(S & STATUS_NEGATIVE); return 2; // BMI
        case 0x50: ADRS_REL(); BRANCH(!(S & STATUS_OVERFLOW)); return 2; // BVC
        case 0x70: ADRS_REL(); BRANCH(S & STATUS_OVERFLOW); return 2; // BVS
        case 0x90: ADRS_REL(); BRANCH(!(S & STATUS_CARRY)); return 2; // BCC
        case 0xB0: ADRS_REL(); BRANCH(S & STATUS_CARRY); return 2; // BCS
        case 0xD0: ADRS_REL(); BRANCH(!(S & STATUS_ZERO)); return 2; // BNE
        case 0xF0: ADRS_REL(); BRANCH(S & STATUS_ZERO); return 2; // BEQ
        // STACK INSTRUCTIONS
        case 0x08: ADRS_IMP(); PHP(); return 3; // PHP
        case 0x28: ADRS_IMP(); PLP(); return 4; // PLP
        case 0x48: ADRS_IMP(); PHA(); return 3; // PHA
        case 0x68: ADRS_IMP(); PLA(); return 4; // PLA
        case 0xBA: ADRS_IMP(); TSX(); return 2; // TSX IMMEDIATE
        case 0x9A: ADRS_IMP(); TXS(); return 2; // TXS IMMEDIATE
        // REGISTER INSTRUCTIONS
        case 0x8A: ADRS_IMP(); T(X,A); return 2; // TXA IMMEDIATE
        case 0x98: ADRS_IMP(); T(Y,A); return 2; // TYA IMMEDIATE
        case 0xA8: ADRS_IMP(); T(A,Y); return 2; // TAY IMMEDIATE
        case 0xAA: ADRS_IMP(); T(A,X); return 2; // TAX IMMEDIATE
        case 0xCA: ADRS_IMP(); DE(X); return 2; // DEX IMMEDIATE
        case 0xE8: ADRS_IMP(); IN(X); return 2; // INX IMMEDIATE
        case 0x88: ADRS_IMP(); DE(Y); return 2; // DEY IMMEDIATE
        case 0xC8: ADRS_IMP(); IN(Y); return 2; // INY IMMEDIATE
        // BITWISE INSTRUCTIONS
        case 0x01: ADRS_INDX(); ORA(); return 6; // ORA X INDIRECT
        case 0x05: ADRS_ZP(); ORA(); return 3; // ORA ZERO PAGE
        case 0x09: ADRS_IMM(); ORA(); return 2; // ORA IMM
        case 0x11: ADRS_INDY(); ORA(); return 5; // ORA INDIRECT Y
        case 0x0D: ADRS_ABS(); ORA(); return 4; // ORA ABSOLUTE
        case 0x19: ADRS_ABS_Y(); ORA(); return 4; // ORA ABSOLUTE INDIRECT Y
        case 0x15: ADRS_ZP_X(); ORA(); return 4; // ORA ZP X
        case 0x1D: ADRS_ABS_X(); ORA(); return 4; // ORA ABS X

        case 0x41: ADRS_INDX(); EOR(); return 6; // EOR X INDIRECT
        case 0x45: ADRS_ZP(); EOR(); return 3; // EOR ZERO PAGE
        case 0x49: ADRS_IMM(); EOR(); return 2; // EOR IMM
        case 0x4D: ADRS_ABS(); EOR(); return 4; // EOR ABSOLUTE
        case 0x51: ADRS_INDY(); EOR(); return 5; // EOR INDIRECT Y
        case 0x59: ADRS_ABS_Y(); EOR(); return 4; // EOR ABSOLUTE INDIRECT Y
        case 0x55: ADRS_ZP_X(); EOR(); return 4; //  EOR ZP X
        case 0x5D: ADRS_ABS_X(); EOR(); return 4; // EOR ABS X

        case 0x61: ADRS_INDX(); ADC(); return 6; // ADC X INDIRECT
        case 0x65: ADRS_ZP(); ADC(); return 3; // ADC ZP
        case 0x69: ADRS_IMM(); ADC(); return 2; // ADC IMM
        case 0x6D: ADRS_ABS(); ADC(); return 4; // ADC ABSOLUTE
        case 0x71: ADRS_INDY(); ADC(); return 5; // ADC INDIRECT Y
        case 0x79: ADRS_ABS_Y(); ADC(); return 4; // ADC ABSOLUTE INDIRECT Y
        case 0x75: ADRS_ZP_X(); ADC(); return 4; //  ADC ZP X
        case 0x7D: ADRS_ABS_X(); ADC(); return 4; // ADC ABS X

        case 0x24: ADRS_ZP();  BIT(); return 3; // BIT ZP
        case 0x2C: ADRS_ABS(); BIT(); return 4; // BIT ABS

        case 0x21: ADRS_INDX(); AND(); return 6; // AND X INDIRECT
        case 0x25: ADRS_ZP(); AND(); return 3; // AND ZERO PAGE
        case 0x29: ADRS_IMM(); AND(); return 2; // AND IMMEDIATE
        case 0x2D: ADRS_ABS(); AND(); return 4; // AND ABSOLUTE
        case 0x31: ADRS_INDY(); AND(); return 5; // AND INDIRECT Y
        case 0x39: ADRS_ABS_Y(); AND(); return 4; // AND ABSOLUTE INDEDEX Y
        case 0x35: ADRS_ZP_X(); AND(); return 4; // AND ZP X
        case 0x3D: ADRS_ABS_X(); AND(); return 4; // AND ABS X

        case 0xC1: ADRS_INDX(); CP(A); return 6; // CP A X INDIRECT
        case 0xC5: ADRS_ZP(); CP(A); return 3; // CP A ZP
        case 0xC9: ADRS_IMM(); CP(A); return 2; // CP A IMMEDIATE
        case 0xCD: ADRS_ABS(); CP(A); return 4; // CP A ABSOLUTE
        case 0xD9: ADRS_ABS_Y(); CP(A); return 4; // CP A ABSOLUTE INDEXED Y
        case 0xD1: ADRS_INDY(); CP(A); return 5; // CP INDIRECT Y
        case 0xD5: ADRS_ZP_X(); CP(A); return 4; // CP ZP X
        case 0xDD: ADRS_ABS_X(); CP(A); return 4; // CP ABS X

        case 0xC0: ADRS_IMM(); CP(Y); return 2; // CP Y IMMEDIATE
        case 0xC4: ADRS_ZP(); CP(Y); return 3; // CP Y ZP
        case 0xCC: ADRS_ABS(); CP(Y); return 4; // CP Y ABSOLUTE

        case 0xE0: ADRS_IMM(); CP(X); return 2; // CP X IMMEDIATE
        case 0xE4: ADRS_ZP(); CP(X); return 3; // CP X ZERO PAGE
        case 0xEC: ADRS_ABS(); CP(X); return 4; // CP X ABSOLUTE

        case 0xE1: ADRS_INDX(); SBC(); return 6; // SBC X INDIRECT
        case 0xE5: ADRS_ZP(); SBC(); return 3; // SBC ZP
        case 0xE9: ADRS_IMM(); SBC(); return 2; //  SBC IMMEDIATE
        case 0xED: ADRS_ABS(); SBC(); return 4; // SBC ABSOLUTE
        case 0xF1: ADRS_INDY(); SBC(); return 5; // SBC INDIRECT Y
        case 0xF9: ADRS_ABS_Y(); SBC(); return 4; // SBC ABSLUTE INDEXED Y
        case 0xF5: ADRS_ZP_X(); SBC(); return 4; // SBC ZP X
        case 0xFD: ADRS_ABS_X(); SBC(); return 4; // SBC ABS X

        case 0x4A: ADRS_ACC(); LSRA(); return 2;  // LSR ACCUMULATOR
        case 0x46: ADRS_ZP(); LSR(); return 5; // LSR ZERO PAGE
        case 0x4E: ADRS_ABS(); LSR(); return 6; // LSR ABSOLUTE
        case 0x56: ADRS_ZP_X(); LSR(); return 6; // LSR ZP X
        case 0x5E: __ADRS_ABS_X(); LSR(); return 7; // LSR ABS X

        case 0x06: ADRS_ZP(); ASL(); return 5; // ASL ZERO PAGE
        case 0x0A: ADRS_ACC(); ASLA(); return 2; // ASL ACCUMULATOR
        case 0x0E: ADRS_ABS(); ASL(); return 6; // ASL ABSOLUTE
        case 0x16: ADRS_ZP_X(); ASL(); return 6; // ASL ZP X
        case 0x1E: __ADRS_ABS_X(); ASL(); return 7; // ASL ABS X

        case 0x66: ADRS_ZP(); ROR(); return 5; // ROR ZERO PAGE
        case 0x6A: ADRS_ACC(); RORA(); return 2; // ROR ACCUMULATOR
        case 0x6E: ADRS_ABS(); ROR(); return 6; // ROR ABSOLUTE
        case 0x76: ADRS_ZP_X(); ROR(); return 6; // ROR ZP X
        case 0x7E: __ADRS_ABS_X(); ROR(); return 7; // ROR ABS X

        case 0x26: ADRS_ZP(); ROL(); return 5; // ROL ZERO PAGE
        case 0x2A: ADRS_ACC(); ROLA(); return 2; // ROL ACCUMULATOR
        case 0x2E: ADRS_ABS(); ROL(); return 6; // ROL ABSOLUTE
        case 0x36: ADRS_ZP_X(); ROL(); return 6; // ROL ZP X
        case 0x3E: __ADRS_ABS_X(); ROL(); return 7; // ROL ABS X

        case 0xC6: ADRS_ZP(); DEC(); return 5; // DEC ZERO PAGE
        case 0xCE: ADRS_ABS(); DEC(); return 6; // DEC ABSOLUTE
        case 0xD6: ADRS_ZP_X(); DEC(); return 6; // DEC ZPX
        case 0xDE: __ADRS_ABS_X(); DEC(); return 7; // DEC ABSOLUTE X

        case 0xE6: ADRS_ZP(); INC(); return 5; // INC ZERO PAGE
        case 0xEE: ADRS_ABS(); INC(); return 6; // INC ABSOLUT
        case 0xF6: ADRS_ZP_X(); INC(); return 6; // INX ZPX
        case 0xFE: __ADRS_ABS_X(); INC(); return 7; // INX ABSOLUTE X

        case 0x40: ADRS_IMP(); RTI(); return 6; // RTI IMPLIED
        default:
            printf("NOT IMPLEMENTED: %02X\n", opcode);
            printf("%04x\t%02X A: %02X X: %02X Y: %02X S: %02X SP: %02X CYC: %lld PPUCTRL: %02x",PC,
                   opcode,
                   A,
                   X,
                   Y,
                   S,
                   SP,
                   nes->total_cycles, nes->ppu.registers.PPUSTATUS);
            nes->cpu.uoc = true;
            break;
    }
    return 0;
}

void cpu_reset(struct nes *nes)
{
    cpu_interrupt(nes, RESET_VECTOR);
}

void cpu_init(struct nes *nes)
{
    memset(&nes->cpu, 0, sizeof(struct cpu));
    nes->cpu.cycles = 0;
    nes->cpu.registers.pc = 0x0000;
    nes->cpu.registers.s = 0x34;
    A = X = Y = 0;
    SP = 0xFD;
}
