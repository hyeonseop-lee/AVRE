// avr.c

#include <stdio.h>
#include <string.h>

#include "avr.h"
#include "inst.h"

struct AVR avr;

void avr_init() {
    memset(avr.sram.bytes, 0, SRAM_SIZE_BYTES);
    memset(avr.flash.bytes, 0, FLASH_SIZE_BYTES);
    avr_reset();
}

void avr_reset() {
    avr.pc = 0;
    avr.cycle = 0;
    memset(avr.sram.regs, 0, REGS_SIZE_BYTES);
    avr.sram.bytes[AVR_REG_SPH] = (SRAM_SIZE_BYTES - 1) >> 8;
    avr.sram.bytes[AVR_REG_SPL] = (SRAM_SIZE_BYTES - 1) & 0xff;
}

void avr_run() {
    uint16_t inst;
    int i;

    avr.sreg.bits = avr_read_byte(AVR_REG_SREG);

    if(avr.irq && avr.sreg.I) {
        for(i = 0; i < IRQ_COUNT && (avr.irq & (1 << i)) == 0; i++);
        avr.irq ^= (1 << i);
        avr_push_word(avr.pc);
        avr.pc = i;
        avr.sreg.I = 0;
        avr_write_byte(AVR_REG_SREG, avr.sreg.bits);
    }

    inst = avr.flash.words[avr.pc++];
    inst_handler[inst](inst);
    avr_write_byte(AVR_REG_SREG, avr.sreg.bits);
}

void avr_irq(int irq) {
    avr.irq |= (1 << irq);
}

uint8_t avr_read_byte(uint16_t addr) {
    if(addr < REGS_SIZE_BYTES && avr.read_handler[addr]) {
        return avr.read_handler[addr](addr, avr.sram.regs[addr]);
    }
    return avr.sram.bytes[addr];
}

void avr_write_byte(uint16_t addr, uint8_t data) {
    if(addr < REGS_SIZE_BYTES && avr.write_handler[addr]) {
        avr.sram.regs[addr] = avr.write_handler[addr](addr, avr.sram.regs[addr]);
    }
    avr.sram.bytes[addr] = data;
}

uint16_t avr_read_word(uint16_t addr) {
    return avr_read_byte(addr) | ((uint16_t)avr_read_byte(addr + 1)) << 8;
}

void avr_write_word(uint16_t addr, uint16_t data) {
    avr_write_byte(addr, data & 0xff);
    avr_write_byte(addr + 1, data >> 8);
}

uint8_t avr_pop_byte() {
    uint16_t sp;
    sp = avr_read_word(AVR_REG_SP);
    avr_write_word(AVR_REG_SP, ++sp);
    return avr_read_byte(sp);
}

void avr_push_byte(uint8_t data) {
    uint16_t sp;
    sp = avr_read_word(AVR_REG_SP);
    avr_write_byte(sp, data);
    avr_write_word(AVR_REG_SP, --sp);
}

uint16_t avr_pop_word() {
    uint16_t sp;
    sp = avr_read_word(AVR_REG_SP);
    avr_write_word(AVR_REG_SP, sp + 2);
    return avr_read_word(sp - 1);
}

void avr_push_word(uint16_t data) {
    uint16_t sp;
    sp = avr_read_word(AVR_REG_SP);
    avr_write_word(sp - 1, data);
    avr_write_word(AVR_REG_SP, sp - 2);
}

void avr_unimplemented(const char *fn) {
    avr.pc--;
    fprintf(stderr, "unimplemented instruction: %s at %x\n", fn + 3, (uint32_t)avr.pc << 1);
    // TODO: unimplemented instruction
}

void avr_illegalinst(uint16_t inst) {
    avr.pc--;
    fprintf(stderr, "illegal instruction: %02x %02x at %x\n", inst & 0xff, inst >> 8, (uint32_t)avr.pc << 1);
    // TODO: illegal instruction
}

void avr_break() {
    fprintf(stderr, "break at %x\n", (uint32_t)(avr.pc - 1) << 1);
    // TODO: break
}
