// instruction.cc

#include "avr.hh"
#include "instruction.hh"

static bool extended_inst(uint16_t inst) {
    return (inst & 0xfe0e) == 0x940e // CALL
        || (inst & 0xfe0e) == 0x940c // JMP
        || (inst & 0xfe0f) == 0x9000 // LDS
        || (inst & 0xfe0f) == 0x9200; // STS
}

static int do_ADC(AVR *avr, uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr->read_byte(r), Rd = avr->read_byte(d), x;
    x = Rd + Rr + (avr->sreg.C ? 1 : 0);
    avr->sreg.H = (((Rr & Rd) | (Rr & ~x) | (~x & Rd)) & 0x08) != 0;
    avr->sreg.V = (((Rd & Rr & ~x) | (~Rd & ~Rr & x)) & 0x80) != 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = x == 0;
    avr->sreg.C = (((Rd & Rr) | (Rr & ~x) | (~x & Rd)) & 0x80) != 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_ADD(AVR *avr, uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr->read_byte(r), Rd = avr->read_byte(d), x;
    x = Rd + Rr;
    avr->sreg.H = (((Rd & Rr) | (Rr & ~x) | (~x & Rd)) & 0x08) != 0;
    avr->sreg.V = (((Rd & Rr & ~x) | (~Rd & ~Rr & x)) & 0x80) != 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = x == 0;
    avr->sreg.C = (((Rd & Rr) | (Rr & ~x) | (~x & Rd)) & 0x80) != 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_ADIW(AVR *avr, uint16_t inst)
{
    // --------KKddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 2) & 0x30);
    uint16_t d = 0x18u + (((inst >> 4) & 0x3) << 1);
    uint16_t Rd = avr->read_word(d), x;
    x = Rd + K;
    avr->sreg.V = ((~Rd & x) & 0x8000) != 0;
    avr->sreg.N = (x & 0x8000) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = x == 0;
    avr->sreg.C = ((~x & Rd) & 0x8000) != 0;
    avr->write_word(d, x);
    return 2;
}

static int do_AND(AVR *avr, uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr->read_byte(r), Rd = avr->read_byte(d), x;
    x = Rr & Rd;
    avr->sreg.V = 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N;
    avr->sreg.Z = x == 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_ANDI(AVR *avr, uint16_t inst)
{
    // ----KKKKddddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 4) & 0xf0);
    uint16_t d = 16 + ((inst >> 4) & 0xf);
    uint8_t Rd = avr->read_byte(d), x;
    x = Rd & K;
    avr->sreg.S = (x & 0x80) != 0;
    avr->sreg.V = 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.Z = x == 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_ASR(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr->read_byte(d), x;
    x = (int8_t)Rd >> 1;
    avr->sreg.C = Rd & 0x01;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.V = avr->sreg.N ^ avr->sreg.C;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = x == 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_BCLR(AVR *avr, uint16_t inst)
{
    // ---------sss----
    uint16_t t = ((inst >> 4) & 0x7);
    avr->sreg.bits &= ~(1 << t);
    return 1;
}

static int do_BLD(AVR *avr, uint16_t inst)
{
    // -------ddddd-bbb
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t b = (inst & 0x7);
    uint8_t Rd = avr->read_byte(d), x;
    x = (Rd & ~(1 << b)) | ((avr->sreg.T ? 1 : 0) << b);
    avr->write_byte(d, x);
    return 1;
}

static int do_BRBC(AVR *avr, uint16_t inst)
{
    // ------kkkkkkksss
    uint16_t k = ((inst >> 3) & 0x7f);
    uint16_t t = (inst & 0x7);
    if((avr->sreg.bits & (1 << t)) == 0) {
        avr->pc += (int8_t)(k << 1) >> 1;
    }
    return 1;
}

static int do_BRBS(AVR *avr, uint16_t inst)
{
    // ------kkkkkkksss
    uint16_t k = ((inst >> 3) & 0x7f);
    uint16_t t = (inst & 0x7);
    if(avr->sreg.bits & (1 << t)) {
        avr->pc += (int8_t)(k << 1) >> 1;
    }
    return 1;
}

static int do_BREAK(AVR *avr, uint16_t inst)
{
    avr->unimplemented(__FUNCTION__);
    return 0;
}

static int do_BSET(AVR *avr, uint16_t inst)
{
    // ---------sss----
    uint16_t t = ((inst >> 4) & 0x7);
    avr->sreg.bits |= 1 << t;
    return 1;
}

static int do_BST(AVR *avr, uint16_t inst)
{
    // -------ddddd-bbb
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t b = (inst & 0x7);
    uint8_t Rd;
    Rd = avr->read_byte(d);
    avr->sreg.T = ((Rd & (1 << b)) != 0);
    return 1;
}

static int do_CALL(AVR *avr, uint16_t inst)
{
    // -------kkkkk---k
    uint16_t k = (inst & 0x1) | ((inst >> 3) & 0x3e);
    k = k << 16 | avr->flash.words[avr->pc++];
    avr->push_word(avr->pc);
    avr->pc = k;
    return 4;
}

static int do_CBI(AVR *avr, uint16_t inst)
{
    // --------AAAAAbbb
    uint16_t A = ((inst >> 3) & 0x1f) + 0x20;
    uint16_t b = (inst & 0x7);
    uint8_t RA = avr->read_byte(A);
    RA &= ~(1 << b);
    avr->write_byte(A, RA);
    return 2;
}

static int do_COM(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr->read_byte(d), x;
    x = ~Rd;
    avr->sreg.V = 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N;
    avr->sreg.Z = x == 0;
    avr->sreg.C = 1;
    avr->write_byte(d, x);
    return 1;
}

static int do_CP(AVR *avr, uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr->read_byte(r), Rd = avr->read_byte(d), x;
    x = Rd - Rr;
    avr->sreg.H = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x08) != 0;
    avr->sreg.V = (((Rd & ~Rr & ~x) | (~Rd & Rr & x)) & 0x80) != 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = x == 0;
    avr->sreg.C = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x80) != 0;
    return 1;
}

static int do_CPC(AVR *avr, uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr->read_byte(r), Rd = avr->read_byte(d), x;
    x = Rd - Rr - (avr->sreg.C ? 1 : 0);
    avr->sreg.H = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x08) != 0;
    avr->sreg.V = (((Rd & ~Rr & ~x) | (~Rd & Rr & x)) & 0x80) != 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z &= x == 0;
    avr->sreg.C = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x80) != 0;
    return 1;
}

static int do_CPI(AVR *avr, uint16_t inst)
{
    // ----KKKKddddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 4) & 0xf0);
    uint16_t d = 16 + ((inst >> 4) & 0xf);
    uint8_t Rd = avr->read_byte(d), x;
    x = Rd - K;
    avr->sreg.H = (((~Rd & K) | (K & x) | (x & ~Rd)) & 0x08) != 0;
    avr->sreg.V = (((Rd & ~K & ~x) | (~Rd & K & x)) & 0x80) != 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = x == 0;
    avr->sreg.C = (((~Rd & K) | (K & x) | (x & ~Rd)) & 0x80) != 0;
    return 1;
}

static int do_CPSE(AVR *avr, uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr->read_byte(r), Rd = avr->read_byte(d);
    if(Rd == Rr) {
        if(extended_inst(avr->flash.words[avr->pc])) {
            avr->pc++;
            return 1;
        }
        avr->pc++;
        return 1;
    }
    return 1;
}

static int do_DEC(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr->read_byte(d);
    Rd--;
    avr->sreg.V = Rd == 0x7f;
    avr->sreg.N = (Rd & 0x80) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = Rd == 0;
    avr->write_byte(d, Rd);
    return 1;
}

static int do_DES(AVR *avr, uint16_t inst)
{
    avr->unimplemented(__FUNCTION__);
    return 0;
}

static int do_EICALL(AVR *avr, uint16_t inst)
{
    avr->unimplemented(__FUNCTION__);
    return 0;
}

static int do_EIJMP(AVR *avr, uint16_t inst)
{
    avr->unimplemented(__FUNCTION__);
    return 0;
}

static int do_ELPM_1(AVR *avr, uint16_t inst)
{
    uint32_t Z = avr->read_byte(AVR_REG_RAMPZ);
    Z = Z << 16 | avr->read_word(AVR_REG_Z);
    avr->write_byte(0, avr->flash.bytes[Z]);
    return 3;
}

static int do_ELPM_2(AVR *avr, uint16_t inst)
{
    uint16_t d = ((inst >> 4) & 0x1f);
    uint32_t Z = avr->read_byte(AVR_REG_RAMPZ);
    Z = Z << 16 | avr->read_word(AVR_REG_Z);
    avr->write_byte(d, avr->flash.bytes[Z]);
    return 3;
}

static int do_ELPM_3(AVR *avr, uint16_t inst)
{
    uint16_t d = ((inst >> 4) & 0x1f);
    uint32_t Z = avr->read_byte(AVR_REG_RAMPZ);
    Z = Z << 16 | avr->read_word(AVR_REG_Z);
    avr->write_byte(d, avr->flash.bytes[Z++]);
    avr->write_word(AVR_REG_Z, Z & 0xffff);
    avr->write_byte(AVR_REG_RAMPZ, Z >> 16);
    return 3;
}

static int do_EOR(AVR *avr, uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr->read_byte(r), Rd = avr->read_byte(d), x;
    x = Rd ^= Rr;
    avr->sreg.S = (x & 0x80) != 0;
    avr->sreg.V = 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.Z = x == 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_FMUL(AVR *avr, uint16_t inst)
{
    avr->unimplemented(__FUNCTION__);
    return 0;
}

static int do_FMULS(AVR *avr, uint16_t inst)
{
    avr->unimplemented(__FUNCTION__);
    return 0;
}

static int do_FMULSU(AVR *avr, uint16_t inst)
{
    avr->unimplemented(__FUNCTION__);
    return 0;
}

static int do_ICALL(AVR *avr, uint16_t inst)
{
    avr->push_word(avr->pc);
    avr->pc = avr->read_word(AVR_REG_Z);
    return 3;
}

static int do_IJMP(AVR *avr, uint16_t inst)
{
    avr->pc = avr->read_word(AVR_REG_Z);
    return 2;
}

static int do_IN(AVR *avr, uint16_t inst)
{
    // -----AAdddddAAAA
    uint16_t A = ((inst & 0xf) | ((inst >> 5) & 0x30)) + 0x20;
    uint16_t d = ((inst >> 4) & 0x1f);
    avr->write_byte(d, avr->read_byte(A));
    return 1;
}

static int do_INC(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr->read_byte(d);
    Rd++;
    avr->sreg.V = Rd == 0x80;
    avr->sreg.N = (Rd & 0x80) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = Rd == 0;
    avr->write_byte(d, Rd);
    return 1;
}

static int do_JMP(AVR *avr, uint16_t inst)
{
    // -------kkkkk---k
    uint16_t k = (inst & 0x1) | ((inst >> 3) & 0x3e);
    k = k << 16 | avr->flash.words[avr->pc++];
    avr->pc = k;
    return 3;
}

static int do_LD_X1(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t X = avr->read_word(AVR_REG_X);
    avr->write_byte(d, avr->read_byte(X));
    return 2;
}

static int do_LD_X2(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t X = avr->read_word(AVR_REG_X);
    avr->write_byte(d, avr->read_byte(X++));
    avr->write_word(AVR_REG_X, X);
    return 2;
}

static int do_LD_X3(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t X = avr->read_word(AVR_REG_X);
    avr->write_byte(d, avr->read_byte(--X));
    avr->write_word(AVR_REG_X, X);
    return 2;
}

static int do_LD_Y2(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t Y = avr->read_word(AVR_REG_Y);
    avr->write_byte(d, avr->read_byte(Y++));
    avr->write_word(AVR_REG_Y, Y);
    return 2;
}

static int do_LD_Y3(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t Y = avr->read_word(AVR_REG_Y);
    avr->write_byte(d, avr->read_byte(--Y));
    avr->write_word(AVR_REG_Y, Y);
    return 2;
}

static int do_LD_Y4(AVR *avr, uint16_t inst)
{
    // --q-qq-ddddd-qqq
    uint16_t q = (inst & 0x7) | ((inst >> 7) & 0x18) | ((inst >> 8) & 0x20);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t Y = avr->read_word(AVR_REG_Y);
    avr->write_byte(d, avr->read_byte(Y + q));
    return 2;
}

static int do_LD_Z2(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t Z = avr->read_word(AVR_REG_Z);
    avr->write_byte(d, avr->read_byte(Z++));
    avr->write_word(AVR_REG_Z, Z);
    return 2;
}

static int do_LD_Z3(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t Z = avr->read_word(AVR_REG_Z);
    avr->write_byte(d, avr->read_byte(--Z));
    avr->write_word(AVR_REG_Z, Z);
    return 2;
}

static int do_LD_Z4(AVR *avr, uint16_t inst)
{
    // --q-qq-ddddd-qqq
    uint16_t q = (inst & 0x7) | ((inst >> 7) & 0x18) | ((inst >> 8) & 0x20);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t Z = avr->read_word(AVR_REG_Z);
    avr->write_byte(d, avr->read_byte(Z + q));
    return 2;
}

static int do_LDI(AVR *avr, uint16_t inst)
{
    // ----KKKKddddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 4) & 0xf0);
    uint16_t d = 16 + ((inst >> 4) & 0xf);
    avr->write_byte(d, K);
    return 1;
}

static int do_LDS(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t k = avr->flash.words[avr->pc++];
    avr->write_byte(d, avr->read_byte(k));
    return 2;
}

static int do_LPM_1(AVR *avr, uint16_t inst)
{
    uint32_t Z = avr->read_byte(AVR_REG_RAMPZ);
    Z = Z << 16 | avr->read_word(AVR_REG_Z);
    avr->write_byte(0, avr->flash.bytes[Z]);
    return 3;
}

static int do_LPM_2(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint32_t Z = avr->read_byte(AVR_REG_RAMPZ);
    Z = Z << 16 | avr->read_word(AVR_REG_Z);
    avr->write_byte(d, avr->flash.bytes[Z]);
    return 3;
}

static int do_LPM_3(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint32_t Z = avr->read_byte(AVR_REG_RAMPZ);
    Z = Z << 16 | avr->read_word(AVR_REG_Z);
    avr->write_byte(d, avr->flash.bytes[Z++]);
    avr->write_word(AVR_REG_Z, Z & 0xffff);
    return 3;
}

static int do_LSR(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr->read_byte(d), x;
    x = Rd >> 1;
    avr->sreg.C = Rd & 0x01;
    avr->sreg.N = 0;
    avr->sreg.V = avr->sreg.N ^ avr->sreg.C;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = x == 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_MOV(AVR *avr, uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    avr->write_byte(d, avr->read_byte(r));
    return 1;
}

static int do_MOVW(AVR *avr, uint16_t inst)
{
    // --------ddddrrrr
    uint16_t d = ((inst >> 4) & 0xf) << 1;
    uint16_t r = (inst & 0xf) << 1;
    avr->write_word(d, avr->read_word(r));
    return 1;
}

static int do_MUL(AVR *avr, uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t x;
    uint8_t Rr = avr->read_byte(r), Rd = avr->read_byte(d);
    x = Rr * Rd;
    avr->write_word(0, x);
    avr->sreg.C = (x & 0x8000) != 0;
    avr->sreg.Z = x == 0;
    return 2;
}

static int do_MULS(AVR *avr, uint16_t inst)
{
    // --------ddddrrrr
    uint16_t d = ((inst >> 4) & 0xf);
    uint16_t r = (inst & 0xf);
    int16_t x;
    int8_t Rr = avr->read_byte(r), Rd = avr->read_byte(d);
    x = Rr * Rd;
    avr->write_word(0, x);
    avr->sreg.C = (x & 0x8000) != 0;
    avr->sreg.Z = x == 0;
    return 2;
}

static int do_MULSU(AVR *avr, uint16_t inst)
{
    // ---------ddd-rrr
    uint16_t d = ((inst >> 4) & 0x7);
    uint16_t r = (inst & 0x7);
    int16_t x;
    uint8_t Rr = avr->read_byte(r);
    int8_t Rd = avr->read_byte(d);
    x = Rr * Rd;
    avr->write_word(0, x);
    avr->sreg.C = (x & 0x8000) != 0;
    avr->sreg.Z = x == 0;
    return 2;
}

static int do_NEG(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr->read_byte(d), x;
    x = -Rd;
    avr->sreg.H = ((x | Rd) & 0x08) != 0;
    avr->sreg.V = x == 0x80;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = x == 0;
    avr->sreg.C = x != 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_NOP(AVR *avr, uint16_t inst)
{
    return 1;
}

static int do_OR(AVR *avr, uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr->read_byte(r), Rd = avr->read_byte(d), x;
    x = Rd | Rr;
    avr->sreg.V = 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N;
    avr->sreg.Z = x == 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_ORI(AVR *avr, uint16_t inst)
{
    // ----KKKKddddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 4) & 0xf0);
    uint16_t d = 16 + ((inst >> 4) & 0xf);
    uint8_t Rd = avr->read_byte(d), x;
    x = Rd | K;
    avr->sreg.S = (x & 0x80) != 0;
    avr->sreg.V = 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.Z = x == 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_OUT(AVR *avr, uint16_t inst)
{
    // -----AArrrrrAAAA
    uint16_t A = ((inst & 0xf) | ((inst >> 5) & 0x30)) + 0x20;
    uint16_t r = ((inst >> 4) & 0x1f);
    avr->write_byte(A, avr->read_byte(r));
    return 1;
}

static int do_POP(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    avr->write_byte(d, avr->pop_byte());
    return 2;
}

static int do_PUSH(AVR *avr, uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    avr->push_byte(avr->read_byte(r));
    return 2;
}

static int do_RCALL(AVR *avr, uint16_t inst)
{
    // ----kkkkkkkkkkkk
    uint16_t k = (inst & 0xfff);
    avr->push_word(avr->pc);
    avr->pc += (int16_t)(k << 4) >> 4;
    return 3;
}

static int do_RET(AVR *avr, uint16_t inst)
{
    avr->pc = avr->pop_word();
    return 4;
}

static int do_RETI(AVR *avr, uint16_t inst)
{
    avr->pc = avr->pop_word();
    avr->sreg.I = 1;
    return 4;
}

static int do_RJMP(AVR *avr, uint16_t inst)
{
    // ----kkkkkkkkkkkk
    uint16_t k = (inst & 0xfff);
    avr->pc += (int16_t)(k << 4) >> 4;
    return 2;
}

static int do_ROR(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr->read_byte(d), x;
    x = (Rd >> 1) | (avr->sreg.C ? 0x80 : 0);
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.V = avr->sreg.N ^ avr->sreg.C;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = x == 0;
    avr->sreg.C = Rd & 0x01;
    avr->write_byte(d, x);
    return 1;
}

static int do_SBC(AVR *avr, uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr->read_byte(r), Rd = avr->read_byte(d), x;
    x = Rd - Rr - (avr->sreg.C ? 1 : 0);
    avr->sreg.H = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x08) != 0;
    avr->sreg.V = (((Rd & ~Rr & ~x) | (~Rd & Rr & x)) & 0x80) != 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z &= x == 0;
    avr->sreg.C = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x80) != 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_SBCI(AVR *avr, uint16_t inst)
{
    // ----KKKKddddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 4) & 0xf0);
    uint16_t d = 16 + ((inst >> 4) & 0xf);
    uint8_t Rd = avr->read_byte(d), x;
    x = Rd - K - (avr->sreg.C ? 1 : 0);
    avr->sreg.H = (((~Rd & K) | (K & x) | (x & ~Rd)) & 0x08) != 0;
    avr->sreg.V = (((Rd & ~K & ~x) | (~Rd & K & x)) & 0x80) != 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z &= x == 0;
    avr->sreg.C = (((~Rd & K) | (K & x) | (x & ~Rd)) & 0x80) != 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_SBI(AVR *avr, uint16_t inst)
{
    // --------AAAAAbbb
    uint16_t A = ((inst >> 3) & 0x1f) + 0x20;
    uint16_t b = (inst & 0x7);
    uint8_t RA = avr->read_byte(A);
    RA |= (1 << b);
    avr->write_byte(A, RA);
    return 2;
}

static int do_SBIC(AVR *avr, uint16_t inst)
{
    // --------AAAAAbbb
    uint16_t A = ((inst >> 3) & 0x1f) + 0x20;
    uint16_t b = (inst & 0x7);
    uint8_t RA = avr->read_byte(A);
    if((RA & (1 << b)) == 0) {
        if(extended_inst(avr->flash.words[avr->pc])) {
            avr->pc++;
            return 1;
        }
        avr->pc++;
        return 1;
    }
    return 1;
}

static int do_SBIS(AVR *avr, uint16_t inst)
{
    // --------AAAAAbbb
    uint16_t A = ((inst >> 3) & 0x1f) + 0x20;
    uint16_t b = (inst & 0x7);
    uint8_t RA = avr->read_byte(A);
    if((RA & (1 << b)) != 0) {
        if(extended_inst(avr->flash.words[avr->pc])) {
            avr->pc++;
            return 1;
        }
        avr->pc++;
        return 1;
    }
    return 1;
}

static int do_SBIW(AVR *avr, uint16_t inst)
{
    // --------KKddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 2) & 0x30);
    uint16_t d = 0x18u + (((inst >> 4) & 0x3) << 1);
    uint16_t Wd = avr->read_word(d), x;
    x = Wd - K;
    avr->sreg.V = ((Wd & ~x) & 0x8000) != 0;
    avr->sreg.N = (x & 0x8000) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = x == 0;
    avr->sreg.C = ((x & ~Wd) & 0x8000) != 0;
    avr->write_word(d, x);
    return 2;
}

static int do_SBRC(AVR *avr, uint16_t inst)
{
    // -------rrrrr-bbb
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t b = (inst & 0x7);
    uint8_t Rr = avr->read_byte(r);
    if((Rr & (1 << b)) == 0) {
        if(extended_inst(avr->flash.words[avr->pc])) {
            avr->pc++;
            return 1;
        }
        avr->pc++;
        return 1;
    }
    return 1;
}

static int do_SBRS(AVR *avr, uint16_t inst)
{
    // -------rrrrr-bbb
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t b = (inst & 0x7);
    uint8_t Rr = avr->read_byte(r);
    if(Rr & (1 << b)) {
        if(extended_inst(avr->flash.words[avr->pc])) {
            avr->pc++;
            return 1;
        }
        avr->pc++;
        return 1;
    }
    return 1;
}

static int do_SLEEP(AVR *avr, uint16_t inst)
{
    avr->unimplemented(__FUNCTION__);
    return 0;
}

static int do_SPM2_1(AVR *avr, uint16_t inst)
{
    avr->unimplemented(__FUNCTION__);
    return 0;
}

static int do_SPM2_2(AVR *avr, uint16_t inst)
{
    avr->unimplemented(__FUNCTION__);
    return 0;
}

static int do_ST_X1(AVR *avr, uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t X = avr->read_word(AVR_REG_X);
    avr->write_byte(X, avr->read_byte(r));
    return 2;
}

static int do_ST_X2(AVR *avr, uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t X = avr->read_word(AVR_REG_X);
    avr->write_byte(X++, avr->read_byte(r));
    avr->write_word(AVR_REG_X, X);
    return 2;
}

static int do_ST_X3(AVR *avr, uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t X = avr->read_word(AVR_REG_X);
    avr->write_byte(--X, avr->read_byte(r));
    avr->write_word(AVR_REG_X, X);
    return 2;
}

static int do_ST_Y2(AVR *avr, uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t Y = avr->read_word(AVR_REG_Y);
    avr->write_byte(Y++, avr->read_byte(r));
    avr->write_word(AVR_REG_Y, Y);
    return 2;
}

static int do_ST_Y3(AVR *avr, uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t Y = avr->read_word(AVR_REG_Y);
    avr->write_byte(--Y, avr->read_byte(r));
    avr->write_word(AVR_REG_Y, Y);
    return 2;
}

static int do_ST_Y4(AVR *avr, uint16_t inst)
{
    // --q-qq-rrrrr-qqq
    uint16_t q = (inst & 0x7) | ((inst >> 7) & 0x18) | ((inst >> 8) & 0x20);
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t Y = avr->read_word(AVR_REG_Y);
    avr->write_byte(Y + q, avr->read_byte(r));
    return 2;
}

static int do_ST_Z2(AVR *avr, uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t Z = avr->read_word(AVR_REG_Z);
    avr->write_byte(Z++, avr->read_byte(r));
    avr->write_word(AVR_REG_Z, Z);
    return 2;
}

static int do_ST_Z3(AVR *avr, uint16_t inst)
{
    // -------rrrrr----
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t Z = avr->read_word(AVR_REG_Z);
    avr->write_byte(--Z, avr->read_byte(r));
    avr->write_word(AVR_REG_Z, Z);
    return 2;
}

static int do_ST_Z4(AVR *avr, uint16_t inst)
{
    // --q-qq-rrrrr-qqq
    uint16_t q = (inst & 0x7) | ((inst >> 7) & 0x18) | ((inst >> 8) & 0x20);
    uint16_t r = ((inst >> 4) & 0x1f);
    uint16_t Z = avr->read_word(AVR_REG_Z);
    avr->write_byte(Z + q, avr->read_byte(r));
    return 2;
}

static int do_STS(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint16_t k = avr->flash.words[avr->pc++];
    avr->write_byte(k, avr->read_byte(d));
    return 2;
}

static int do_SUB(AVR *avr, uint16_t inst)
{
    // ------rdddddrrrr
    uint16_t r = (inst & 0xf) | ((inst >> 5) & 0x10);
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rr = avr->read_byte(r), Rd = avr->read_byte(d), x;
    x = Rd - Rr;
    avr->sreg.H = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x08) != 0;
    avr->sreg.V = (((Rd & ~Rr & ~x) | (~Rd & Rr & x)) & 0x80) != 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = x == 0;
    avr->sreg.C = (((~Rd & Rr) | (Rr & x) | (x & ~Rd)) & 0x80) != 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_SUBI(AVR *avr, uint16_t inst)
{
    // ----KKKKddddKKKK
    uint16_t K = (inst & 0xf) | ((inst >> 4) & 0xf0);
    uint16_t d = 16 + ((inst >> 4) & 0xf);
    uint8_t Rd = avr->read_byte(d), x;
    x = Rd - K;
    avr->sreg.H = (((~Rd & K) | (K & x) | (x & ~Rd)) & 0x08) != 0;
    avr->sreg.V = (((Rd & ~K & ~x) | (~Rd & K & x)) & 0x80) != 0;
    avr->sreg.N = (x & 0x80) != 0;
    avr->sreg.S = avr->sreg.N ^ avr->sreg.V;
    avr->sreg.Z = x == 0;
    avr->sreg.C = (((~Rd & K) | (K & x) | (x & ~Rd)) & 0x80) != 0;
    avr->write_byte(d, x);
    return 1;
}

static int do_SWAP(AVR *avr, uint16_t inst)
{
    // -------ddddd----
    uint16_t d = ((inst >> 4) & 0x1f);
    uint8_t Rd = avr->read_byte(d);
    Rd = (Rd << 4) | (Rd >> 4);
    avr->write_byte(d, Rd);
    return 1;
}

static int do_WDR(AVR *avr, uint16_t inst)
{
    avr->unimplemented(__FUNCTION__);
    return 0;
}

#include "instruction_handler.hh"
