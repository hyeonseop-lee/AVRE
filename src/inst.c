// inst.c

#include "avr.h"
#include "inst.h"

static int extended_inst(uint16_t inst) {
    return (inst & 0xfe0e) == 0x940e // CALL
        || (inst & 0xfe0e) == 0x940c // JMP
        || (inst & 0xfe0f) == 0x9000 // LDS
        || (inst & 0xfe0f) == 0x9200; // STS
}

static void do_ADC(uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr_read_byte(r), Rd = avr_read_byte(d), x;
    x = Rd + Rr + (avr.sreg.C ? 1 : 0);
    avr.sreg.H = (((Rr & Rd) | (Rr & ~x) | (~x & Rd)) & 0x08) != 0;
    avr.sreg.V = (((Rd & Rr & ~x) | (~Rd & ~Rr & x)) & 0x80) != 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = x == 0;
    avr.sreg.C = (((Rd & Rr) | (Rr & ~x) | (~x & Rd)) & 0x80) != 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_ADD(uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr_read_byte(r), Rd = avr_read_byte(d), x;
    x = Rd + Rr;
    avr.sreg.H = (((Rd & Rr) | (Rr & ~x) | (~x & Rd)) & 0x08) != 0;
    avr.sreg.V = (((Rd & Rr & ~x) | (~Rd & ~Rr & x)) & 0x80) != 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = x == 0;
    avr.sreg.C = (((Rd & Rr) | (Rr & ~x) | (~x & Rd)) & 0x80) != 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_ADIW(uint16_t inst)
{
    // --------KKddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 2) & 0x30);
    uint16_t d = ((inst >> 4) & 0x3);
    uint16_t Rd = avr_read_word(d), x;
    x = Rd + K;
    avr.sreg.V = ((~Rd & x) & 0x8000) != 0;
    avr.sreg.N = (x & 0x8000) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = x == 0;
    avr.sreg.C = ((~x & Rd) & 0x8000) != 0;
    avr_write_word(d, x);
    avr.cycle += 2;
}

static void do_AND(uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr_read_byte(r), Rd = avr_read_byte(d), x;
    x = Rr & Rd;
    avr.sreg.V = 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N;
    avr.sreg.Z = x == 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_ANDI(uint16_t inst)
{
    // ----KKKKddddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 4) & 0xf0);
    uint16_t d = 16 + ((inst >> 4) & 0xf);
    uint8_t Rd = avr_read_byte(d), x;
    x = Rd & K;
    avr.sreg.S = (x & 0x80) != 0;
    avr.sreg.V = 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.Z = x == 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_ASR(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr_read_byte(d), x;
    x = (int8_t)Rd >> 1;
    avr.sreg.C = Rd & 0x01;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.V = avr.sreg.N ^ avr.sreg.C;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = x == 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_BCLR(uint16_t inst)
{
    // ---------sss----
    uint16_t t = ((inst >> 4) & 0x7);
    avr.sreg.bits &= ~(1 << t);
    avr.cycle++;
}

static void do_BLD(uint16_t inst)
{
    // -------ddddd-bbb
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t b = (inst & 0x7);
    uint8_t Rd = avr_read_byte(d), x;
    x = (Rd & ~(1 << b)) | ((avr.sreg.T ? 1 : 0) << b);
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_BRBC(uint16_t inst)
{
    // ------kkkkkkksss
    uint16_t k = ((inst >> 3) & 0x7f);
    uint16_t t = (inst & 0x7);
    if((avr.sreg.bits & (1 << t)) == 0) {
        avr.pc += (int8_t)(k << 1) >> 1;
    }
    avr.cycle++;
}

static void do_BRBS(uint16_t inst)
{
    // ------kkkkkkksss
    uint16_t k = ((inst >> 3) & 0x7f);
    uint16_t t = (inst & 0x7);
    if(avr.sreg.bits & (1 << t)) {
        avr.pc += (int8_t)(k << 1) >> 1;
    }
    avr.cycle++;
}

static void do_BREAK(uint16_t inst)
{
    avr_break();
    avr.cycle++;
}

static void do_BSET(uint16_t inst)
{
    // ---------sss----
    uint16_t t = ((inst >> 4) & 0x7);
    avr.sreg.bits |= 1 << t;
    avr.cycle++;
}

static void do_BST(uint16_t inst)
{
    // -------ddddd-bbb
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t b = (inst & 0x7);
    uint8_t Rd;
    Rd = avr_read_byte(d);
    avr.sreg.T = ((Rd & (1 << b)) != 0);
    avr.cycle++;
}

static void do_CALL(uint16_t inst)
{
    // -------kkkkk---k
    uint16_t k = (inst & 0x1) | ((inst >> 3) & 0x3e);
    k = k << 16 | avr.flash.words[avr.pc++];
    avr_push_word(avr.pc);
    avr.pc = k;
    avr.cycle += 4;
}

static void do_CBI(uint16_t inst)
{
    // --------AAAAAbbb
    uint16_t A = ((inst >> 3) & 0x1f) + 0x20;
    uint16_t b = (inst & 0x7);
    uint8_t RA = avr_read_byte(A);
    RA &= ~(1 << b);
    avr_write_byte(A, RA);
    avr.cycle += 2;
}

static void do_COM(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr_read_byte(d), x;
    x = ~Rd;
    avr.sreg.V = 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N;
    avr.sreg.Z = x == 0;
    avr.sreg.C = 1;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_CP(uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr_read_byte(r), Rd = avr_read_byte(d), x;
    x = Rd - Rr;
    avr.sreg.H = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x08) != 0;
    avr.sreg.V = (((Rd & ~Rr & ~x) | (~Rd & Rr & x)) & 0x80) != 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = x == 0;
    avr.sreg.C = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x80) != 0;
    avr.cycle++;
}

static void do_CPC(uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr_read_byte(r), Rd = avr_read_byte(d), x;
    x = Rd - Rr - (avr.sreg.C ? 1 : 0);
    avr.sreg.H = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x08) != 0;
    avr.sreg.V = (((Rd & ~Rr & ~x) | (~Rd & Rr & x)) & 0x80) != 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z &= x == 0;
    avr.sreg.C = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x80) != 0;
    avr.cycle++;
}

static void do_CPI(uint16_t inst)
{
    // ----KKKKddddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 4) & 0xf0);
    uint16_t d = 16 + ((inst >> 4) & 0xf);
    uint8_t Rd = avr_read_byte(d), x;
    x = Rd - K;
    avr.sreg.H = (((~Rd & K) | (K & x) | (x & ~Rd)) & 0x08) != 0;
    avr.sreg.V = (((Rd & ~K & ~x) | (~Rd & K & x)) & 0x80) != 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = x == 0;
    avr.sreg.C = (((~Rd & K) | (K & x) | (x & ~Rd)) & 0x80) != 0;
    avr.cycle++;
}

static void do_CPSE(uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr_read_byte(r), Rd = avr_read_byte(d);
    if(Rd == Rr) {
        if(extended_inst(avr.flash.words[avr.pc])) {
            avr.pc++;
            avr.cycle++;
        }
        avr.pc++;
        avr.cycle++;
    }
    avr.cycle++;
}

static void do_DEC(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr_read_byte(d);
    Rd--;
    avr.sreg.V = Rd == 0x7f;
    avr.sreg.N = (Rd & 0x80) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = Rd == 0;
    avr_write_byte(d, Rd);
    avr.cycle++;
}

static void do_DES(uint16_t inst)
{
    avr_unimplemented(__FUNCTION__);
}

static void do_EICALL(uint16_t inst)
{
    avr_unimplemented(__FUNCTION__);
}

static void do_EIJMP(uint16_t inst)
{
    avr_unimplemented(__FUNCTION__);
}

static void do_ELPM_1(uint16_t inst)
{
    uint32_t Z = avr_read_byte(AVR_REG_RAMPZ);
    Z = Z << 16 | avr_read_word(AVR_REG_Z);
    avr_write_byte(0, avr.flash.bytes[Z]);
    avr.cycle += 3;
}

static void do_ELPM_2(uint16_t inst)
{
    uint16_t d = ((inst >> 4) & 0x1f);
    uint32_t Z = avr_read_byte(AVR_REG_RAMPZ);
    Z = Z << 16 | avr_read_word(AVR_REG_Z);
    avr_write_byte(d, avr.flash.bytes[Z]);
    avr.cycle += 3;
}

static void do_ELPM_3(uint16_t inst)
{
    uint16_t d = ((inst >> 4) & 0x1f);
    uint32_t Z = avr_read_byte(AVR_REG_RAMPZ);
    Z = Z << 16 | avr_read_word(AVR_REG_Z);
    avr_write_byte(d, avr.flash.bytes[Z++]);
    avr_write_word(AVR_REG_Z, Z & 0xffff);
    avr_write_byte(AVR_REG_RAMPZ, Z >> 16);
    avr.cycle += 3;
}

static void do_EOR(uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr_read_byte(r), Rd = avr_read_byte(d), x;
    x = Rd ^= Rr;
    avr.sreg.S = (x & 0x80) != 0;
    avr.sreg.V = 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.Z = x == 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_FMUL(uint16_t inst)
{
    avr_unimplemented(__FUNCTION__);
}

static void do_FMULS(uint16_t inst)
{
    avr_unimplemented(__FUNCTION__);
}

static void do_FMULSU(uint16_t inst)
{
    avr_unimplemented(__FUNCTION__);
}

static void do_ICALL(uint16_t inst)
{
    avr_push_word(avr.pc);
    avr.pc = avr_read_word(AVR_REG_Z);
    avr.cycle += 3;
}

static void do_IJMP(uint16_t inst)
{
    avr.pc = avr_read_word(AVR_REG_Z);
    avr.cycle += 2;
}

static void do_IN(uint16_t inst)
{
    // -----AAdddddAAAA
    uint16_t A = ((inst & 0xf) | ((inst >> 5) & 0x30)) + 0x20;
    uint16_t d = ((inst >> 4) & 0x1f);
    avr_write_byte(d, avr_read_byte(A));
    avr.cycle++;
}

static void do_INC(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr_read_byte(d);
    Rd++;
    avr.sreg.V = Rd == 0x80;
    avr.sreg.N = (Rd & 0x80) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = Rd == 0;
    avr_write_byte(d, Rd);
    avr.cycle++;
}

static void do_JMP(uint16_t inst)
{
    // -------kkkkk---k
    uint16_t k = (inst & 0x1) | ((inst >> 3) & 0x3e);
    k = k << 16 | avr.flash.words[avr.pc++];
    avr.pc = k;
    avr.cycle += 3;
}

static void do_LD_X1(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t X = avr_read_word(AVR_REG_X);
    avr_write_byte(d, avr_read_byte(X));
    avr.cycle += 2;
}

static void do_LD_X2(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t X = avr_read_word(AVR_REG_X);
    avr_write_byte(d, avr_read_byte(X++));
    avr_write_word(AVR_REG_X, X);
    avr.cycle += 2;
}

static void do_LD_X3(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t X = avr_read_word(AVR_REG_X);
    avr_write_byte(d, avr_read_byte(--X));
    avr_write_word(AVR_REG_X, X);
    avr.cycle += 2;
}

static void do_LD_Y2(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t Y = avr_read_word(AVR_REG_Y);
    avr_write_byte(d, avr_read_byte(Y++));
    avr_write_word(AVR_REG_Y, Y);
    avr.cycle += 2;
}

static void do_LD_Y3(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t Y = avr_read_word(AVR_REG_Y);
    avr_write_byte(d, avr_read_byte(--Y));
    avr_write_word(AVR_REG_Y, Y);
    avr.cycle += 2;
}

static void do_LD_Y4(uint16_t inst)
{
    // --q-qq-ddddd-qqq
    uint16_t q = (inst & 0x7) | ((inst >> 7) & 0x18) | ((inst >> 8) & 0x20);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t Y = avr_read_word(AVR_REG_Y);
    avr_write_byte(d, avr_read_byte(Y + q));
    avr.cycle += 2;
}

static void do_LD_Z2(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t Z = avr_read_word(AVR_REG_Z);
    avr_write_byte(d, avr_read_byte(Z++));
    avr_write_word(AVR_REG_Z, Z);
    avr.cycle += 2;
}

static void do_LD_Z3(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t Z = avr_read_word(AVR_REG_Z);
    avr_write_byte(d, avr_read_byte(--Z));
    avr_write_word(AVR_REG_Z, Z);
    avr.cycle += 2;
}

static void do_LD_Z4(uint16_t inst)
{
    // --q-qq-ddddd-qqq
    uint16_t q = (inst & 0x7) | ((inst >> 7) & 0x18) | ((inst >> 8) & 0x20);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t Z = avr_read_word(AVR_REG_Z);
    avr_write_byte(d, avr_read_byte(Z + q));
    avr.cycle += 2;
}

static void do_LDI(uint16_t inst)
{
    // ----KKKKddddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 4) & 0xf0);
    uint16_t d = 16 + ((inst >> 4) & 0xf);
    avr_write_byte(d, K);
    avr.cycle++;
}

static void do_LDS(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t k = avr.flash.words[avr.pc++];
    avr_write_byte(d, avr_read_byte(k));
    avr.cycle += 2;
}

static void do_LPM_1(uint16_t inst)
{
    uint32_t Z = avr_read_byte(AVR_REG_RAMPZ);
    Z = Z << 16 | avr_read_word(AVR_REG_Z);
    avr_write_byte(0, avr.flash.bytes[Z]);
    avr.cycle += 3;
}

static void do_LPM_2(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint32_t Z = avr_read_byte(AVR_REG_RAMPZ);
    Z = Z << 16 | avr_read_word(AVR_REG_Z);
    avr_write_byte(d, avr.flash.bytes[Z]);
    avr.cycle += 3;
}

static void do_LPM_3(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint32_t Z = avr_read_byte(AVR_REG_RAMPZ);
    Z = Z << 16 | avr_read_word(AVR_REG_Z);
    avr_write_byte(d, avr.flash.bytes[Z++]);
    avr_write_word(AVR_REG_Z, Z & 0xffff);
    avr.cycle += 3;
}

static void do_LSR(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr_read_byte(d), x;
    x = Rd >> 1;
    avr.sreg.C = Rd & 0x01;
    avr.sreg.N = 0;
    avr.sreg.V = avr.sreg.N ^ avr.sreg.C;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = x == 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_MOV(uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    avr_write_byte(d, avr_read_byte(r));
    avr.cycle++;
}

static void do_MOVW(uint16_t inst)
{
    // --------ddddrrrr
    uint16_t d = ((inst >> 4) & 0xf);
    uint16_t r = (inst & 0xf);
    avr_write_word(d, avr_read_word(r));
    avr.cycle++;
}

static void do_MUL(uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t x;
    uint8_t Rr = avr_read_byte(r), Rd = avr_read_byte(d);
    x = Rr * Rd;
    avr_write_word(0, x);
    avr.sreg.C = (x & 0x8000) != 0;
    avr.sreg.Z = x == 0;
    avr.cycle += 2;
}

static void do_MULS(uint16_t inst)
{
    // --------ddddrrrr
    uint16_t d = ((inst >> 4) & 0xf);
    uint16_t r = (inst & 0xf);
    int16_t x;
    int8_t Rr = avr_read_byte(r), Rd = avr_read_byte(d);
    x = Rr * Rd;
    avr_write_word(0, x);
    avr.sreg.C = (x & 0x8000) != 0;
    avr.sreg.Z = x == 0;
    avr.cycle += 2;
}

static void do_MULSU(uint16_t inst)
{
    // ---------ddd-rrr
    uint16_t d = ((inst >> 4) & 0x7);
    uint16_t r = (inst & 0x7);
    int16_t x;
    uint8_t Rr = avr_read_byte(r);
    int8_t Rd = avr_read_byte(d);
    x = Rr * Rd;
    avr_write_word(0, x);
    avr.sreg.C = (x & 0x8000) != 0;
    avr.sreg.Z = x == 0;
    avr.cycle += 2;
}

static void do_NEG(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr_read_byte(d), x;
    x = -Rd;
    avr.sreg.H = ((x | Rd) & 0x08) != 0;
    avr.sreg.V = x == 0x80;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = x == 0;
    avr.sreg.C = x != 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_NOP(uint16_t inst)
{
    avr.cycle++;
}

static void do_OR(uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr_read_byte(r), Rd = avr_read_byte(d), x;
    x = Rd | Rr;
    avr.sreg.V = 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N;
    avr.sreg.Z = x == 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_ORI(uint16_t inst)
{
    // ----KKKKddddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 4) & 0xf0);
    uint16_t d = 16 + ((inst >> 4) & 0xf);
    uint8_t Rd = avr_read_byte(d), x;
    x = Rd | K;
    avr.sreg.S = (x & 0x80) != 0;
    avr.sreg.V = 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.Z = x == 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_OUT(uint16_t inst)
{
    // -----AArrrrrAAAA
    uint16_t A = ((inst & 0xf) | ((inst >> 5) & 0x30)) + 0x20;
    uint16_t r = ((inst >> 4) & 0x1f);
    avr_write_byte(A, avr_read_byte(r));
    avr.cycle++;
}

static void do_POP(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    avr_write_byte(d, avr_pop_byte());
    avr.cycle += 2;
}

static void do_PUSH(uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    avr_push_byte(avr_read_byte(r));
    avr.cycle += 2;
}

static void do_RCALL(uint16_t inst)
{
    // ----kkkkkkkkkkkk
    uint16_t k = (inst & 0xfff);
    avr_push_word(avr.pc);
    avr.pc += (int16_t)(k << 4) >> 4;
    avr.cycle += 3;
}

static void do_RET(uint16_t inst)
{
    avr.pc = avr_pop_word();
    avr.cycle += 4;
}

static void do_RETI(uint16_t inst)
{
    avr.pc = avr_pop_word();
    avr.sreg.I = 1;
    avr.cycle += 4;
}

static void do_RJMP(uint16_t inst)
{
    // ----kkkkkkkkkkkk
    uint16_t k = (inst & 0xfff);
    avr.pc += (int16_t)(k << 4) >> 4;
    avr.cycle += 2;
}

static void do_ROR(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr_read_byte(d), x;
    x = (Rd >> 1) | (avr.sreg.C ? 0x80 : 0);
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.V = avr.sreg.N ^ avr.sreg.C;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = x == 0;
    avr.sreg.C = Rd & 0x01;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_SBC(uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr_read_byte(r), Rd = avr_read_byte(d), x;
    x = Rd - Rr - (avr.sreg.C ? 1 : 0);
    avr.sreg.H = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x08) != 0;
    avr.sreg.V = (((Rd & ~Rr & ~x) | (~Rd & Rr & x)) & 0x80) != 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z &= x == 0;
    avr.sreg.C = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x80) != 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_SBCI(uint16_t inst)
{
    // ----KKKKddddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 4) & 0xf0);
    uint16_t d = 16 + ((inst >> 4) & 0xf);
    uint8_t Rd = avr_read_byte(d), x;
    x = Rd - K - (avr.sreg.C ? 1 : 0);
    avr.sreg.H = (((~Rd & K) | (K & x) | (x & ~Rd)) & 0x08) != 0;
    avr.sreg.V = (((Rd & ~K & ~x) | (~Rd & K & x)) & 0x80) != 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z &= x == 0;
    avr.sreg.C = (((~Rd & K) | (K & x) | (x & ~Rd)) & 0x80) != 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_SBI(uint16_t inst)
{
    // --------AAAAAbbb
    uint16_t A = ((inst >> 3) & 0x1f) + 0x20;
    uint16_t b = (inst & 0x7);
    uint8_t RA = avr_read_byte(A);
    RA |= (1 << b);
    avr_write_byte(A, RA);
    avr.cycle += 2;
}

static void do_SBIC(uint16_t inst)
{
    // --------AAAAAbbb
    uint16_t A = ((inst >> 3) & 0x1f) + 0x20;
    uint16_t b = (inst & 0x7);
    uint8_t RA = avr_read_byte(A);
    if((RA & (1 << b)) == 0) {
        if(extended_inst(avr.flash.words[avr.pc])) {
            avr.pc++;
            avr.cycle++;
        }
        avr.pc++;
        avr.cycle++;
    }
    avr.cycle++;
}

static void do_SBIS(uint16_t inst)
{
    // --------AAAAAbbb
    uint16_t A = ((inst >> 3) & 0x1f) + 0x20;
    uint16_t b = (inst & 0x7);
    uint8_t RA = avr_read_byte(A);
    if((RA & (1 << b)) != 0) {
        if(extended_inst(avr.flash.words[avr.pc])) {
            avr.pc++;
            avr.cycle++;
        }
        avr.pc++;
        avr.cycle++;
    }
    avr.cycle++;
}

static void do_SBIW(uint16_t inst)
{
    // --------KKddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 2) & 0x30);
    uint16_t d = ((inst >> 4) & 0x3);
    uint16_t Wd = avr_read_word(d), x;
    x = Wd - K;
    avr.sreg.V = ((Wd & ~x) & 0x8000) != 0;
    avr.sreg.N = (x & 0x8000) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = x == 0;
    avr.sreg.C = ((x & ~Wd) & 0x8000) != 0;
    avr_write_word(d, x);
    avr.cycle += 2;
}

static void do_SBRC(uint16_t inst)
{
    // -------rrrrr-bbb
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t b = (inst & 0x7);
    uint8_t Rr = avr_read_byte(r);
    if((Rr & (1 << b)) == 0) {
        if(extended_inst(avr.flash.words[avr.pc])) {
            avr.pc++;
            avr.cycle++;
        }
        avr.pc++;
        avr.cycle++;
    }
    avr.cycle++;
}

static void do_SBRS(uint16_t inst)
{
    // -------rrrrr-bbb
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t b = (inst & 0x7);
    uint8_t Rr = avr_read_byte(r);
    if(Rr & (1 << b)) {
        if(extended_inst(avr.flash.words[avr.pc])) {
            avr.pc++;
            avr.cycle++;
        }
        avr.pc++;
        avr.cycle++;
    }
    avr.cycle++;
}

static void do_SLEEP(uint16_t inst)
{
    avr_unimplemented(__FUNCTION__);
}

static void do_SPM2_1(uint16_t inst)
{
    avr_unimplemented(__FUNCTION__);
}

static void do_SPM2_2(uint16_t inst)
{
    avr_unimplemented(__FUNCTION__);
}

static void do_ST_X1(uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t X = avr_read_word(AVR_REG_X);
    avr_write_byte(r, avr_read_byte(X));
    avr.cycle += 2;
}

static void do_ST_X2(uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t X = avr_read_word(AVR_REG_X);
    avr_write_byte(r, avr_read_byte(X++));
    avr_write_word(AVR_REG_X, X);
    avr.cycle += 2;
}

static void do_ST_X3(uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t X = avr_read_word(AVR_REG_X);
    avr_write_byte(r, avr_read_byte(--X));
    avr_write_word(AVR_REG_X, X);
    avr.cycle += 2;
}

static void do_ST_Y2(uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t Y = avr_read_word(AVR_REG_Y);
    avr_write_byte(r, avr_read_byte(Y++));
    avr_write_word(AVR_REG_Y, Y);
    avr.cycle += 2;
}

static void do_ST_Y3(uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t Y = avr_read_word(AVR_REG_Y);
    avr_write_byte(r, avr_read_byte(--Y));
    avr_write_word(AVR_REG_Y, Y);
    avr.cycle += 2;
}

static void do_ST_Y4(uint16_t inst)
{
    // --q-qq-rrrrr-qqq
    uint16_t q = (inst & 0x7) | ((inst >> 7) & 0x18) | ((inst >> 8) & 0x20);
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t Y = avr_read_word(AVR_REG_Y);
    avr_write_byte(r, avr_read_byte(Y + q));
    avr.cycle += 2;
}

static void do_ST_Z2(uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t Z = avr_read_word(AVR_REG_Z);
    avr_write_byte(r, avr_read_byte(Z++));
    avr_write_word(AVR_REG_Z, Z);
    avr.cycle += 2;
}

static void do_ST_Z3(uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t Z = avr_read_word(AVR_REG_Z);
    avr_write_byte(r, avr_read_byte(--Z));
    avr_write_word(AVR_REG_Z, Z);
    avr.cycle += 2;
}

static void do_ST_Z4(uint16_t inst)
{
    // --q-qq-rrrrr-qqq
    uint16_t q = (inst & 0x7) | ((inst >> 7) & 0x18) | ((inst >> 8) & 0x20);
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t Z = avr_read_word(AVR_REG_Z);
    avr_write_byte(r, avr_read_byte(Z + q));
    avr.cycle += 2;
}

static void do_STS(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t k = avr.flash.words[avr.pc++];
    avr_write_byte(k, avr_read_byte(d));
    avr.cycle += 2;
}

static void do_SUB(uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr_read_byte(r), Rd = avr_read_byte(d), x;
    x = Rd - Rr;
    avr.sreg.H = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x08) != 0;
    avr.sreg.V = (((Rd & ~Rr & ~x) | (~Rd & Rr & x)) & 0x80) != 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = x == 0;
    avr.sreg.C = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x80) != 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_SUBI(uint16_t inst)
{
    // ----KKKKddddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 4) & 0xf0);
    uint16_t d = 16 + ((inst >> 4) & 0xf);
    uint8_t Rd = avr_read_byte(d), x;
    x = Rd - K;
    avr.sreg.H = (((~Rd & K) | (K & x) | (x & ~Rd)) & 0x08) != 0;
    avr.sreg.V = (((Rd & ~K & ~x) | (~Rd & K & x)) & 0x80) != 0;
    avr.sreg.N = (x & 0x80) != 0;
    avr.sreg.S = avr.sreg.N ^ avr.sreg.V;
    avr.sreg.Z = x == 0;
    avr.sreg.C = (((~Rd & K) | (K & x) | (x & ~Rd)) & 0x80) != 0;
    avr_write_byte(d, x);
    avr.cycle++;
}

static void do_SWAP(uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr_read_byte(d);
    Rd = (Rd << 4) | (Rd >> 4);
    avr_write_byte(d, Rd);
    avr.cycle++;
}

static void do_WDR(uint16_t inst)
{
    avr_unimplemented(__FUNCTION__);
}

#include "inst_handler.h"
