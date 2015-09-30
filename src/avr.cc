// avr.cc

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "avr.hh"
#include "instruction.hh"

AVR::AVR(const char *fn, const char *tp)
    : Module()
{
    memset(sram.bytes, 0, SRAM_SIZE_BYTES);
    memset(flash.bytes, 0, FLASH_SIZE_BYTES);

    if(strcasecmp(tp, "elf") == 0)
    {
        fprintf(stderr, "unsupported file type -- '%s'\n\n", tp);
        exit(1);
    }
    else if(strcasecmp(tp, "ihex") == 0)
    {
        load_ihex(fn);
    }
    else if(strcasecmp(tp, "bin") == 0)
    {
        load_bin(fn);
    }
    else
    {
        fprintf(stderr, "unknown file type -- '%s'\n\n", tp);
        exit(1);
    }
}

AVR::~AVR()
{
}

void AVR::initialize()
{
    reset();
}

void AVR::reset()
{
    pc = 0;
    cycle = 0;
    irq = 0;
    memset(sram.regs, 0, REGS_SIZE_BYTES);
    sram.bytes[AVR_REG_SPH] = (SRAM_SIZE_BYTES - 1) >> 8;
    sram.bytes[AVR_REG_SPL] = (SRAM_SIZE_BYTES - 1) & 0xff;
}

void AVR::raise_irq(int num)
{
    irq |= (1 << num);
}

void AVR::register_handler(uint16_t reg, AVR::access_handler read, AVR::access_handler write)
{
    read_handler[reg] = read;
    write_handler[reg] = write;
}

void AVR::process()
{
    uint16_t inst;
    int i;

    sreg.bits = read_byte(AVR_REG_SREG);

    if(irq && sreg.I)
    {
        for(i = 0; i < IRQ_COUNT && (irq & (1u << i)) == 0; i++);
        irq ^= (1u << i);
        push_word(pc);
        pc = i;
        sreg.I = 0;
        write_byte(AVR_REG_SREG, sreg.bits);
    }

    inst = flash.words[pc++];
    instructions[inst](this, inst);
    write_byte(AVR_REG_SREG, sreg.bits);
}

uint8_t AVR::read_byte(uint16_t addr)
{
    if(addr < REGS_SIZE_BYTES && read_handler[addr])
    {
        return read_handler[addr](this, addr, sram.regs[addr]);
    }
    return sram.bytes[addr];
}

void AVR::write_byte(uint16_t addr, uint8_t data)
{
    if(addr < REGS_SIZE_BYTES && write_handler[addr])
    {
        data = write_handler[addr](this, addr, data);
    }
    sram.bytes[addr] = data;
}

uint16_t AVR::read_word(uint16_t addr)
{
    return (uint16_t)read_byte(addr) | (((uint16_t)read_byte(addr + 1)) << 8);
}

void AVR::write_word(uint16_t addr, uint16_t data)
{
    write_byte(addr, data & 0xff);
    write_byte(addr + 1, data >> 8);
}

uint8_t AVR::pop_byte()
{
    uint16_t sp;
    sp = read_word(AVR_REG_SP);
    write_word(AVR_REG_SP, ++sp);
    return read_byte(sp);
}

void AVR::push_byte(uint8_t data)
{
    uint16_t sp;
    sp = read_word(AVR_REG_SP);
    write_byte(sp, data);
    write_word(AVR_REG_SP, --sp);
}

uint16_t AVR::pop_word()
{
    uint16_t sp;
    sp = read_word(AVR_REG_SP);
    write_word(AVR_REG_SP, sp + 2);
    return read_word(sp + 1);
}

void AVR::push_word(uint16_t data)
{
    uint16_t sp;
    sp = read_word(AVR_REG_SP);
    write_word(sp - 1, data);
    write_word(AVR_REG_SP, sp - 2);
}

void AVR::unimplemented(const char *fn)
{
    pc--;
    fprintf(stderr, "unimplemented instruction: %s at %x\n", fn + 3, (uint32_t)pc << 1);
    // TODO: unimplemented instruction
}

void AVR::illegalinst(uint16_t inst)
{
    pc--;
    fprintf(stderr, "illegal instruction: %02x %02x at %x\n", inst & 0xff, inst >> 8, (uint32_t)pc << 1);
    // TODO: illegal instruction
}
