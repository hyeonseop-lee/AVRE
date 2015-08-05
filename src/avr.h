// avr.h

#ifndef AVRE_AVR_H
#define AVRE_AVR_H

#include <stdint.h>

#define SRAM_SIZE_BYTES (0x10000u)
#define REGS_SIZE_BYTES (0x100u)

#define AVR_REG_SREG    (0x5fu)
#define AVR_REG_SP      (0x5du)
#define AVR_REG_SPL     (0x5du)
#define AVR_REG_SPH     (0x5eu)
#define AVR_REG_X       (26u)
#define AVR_REG_Y       (28u)
#define AVR_REG_Z       (30u)
#define AVR_REG_RAMPZ   (0x5cu)

#define FLASH_SIZE_BYTES (0x20000u)
#define FLASH_SIZE_WORDS (0x10000u)

typedef uint8_t (*avr_regs_func)(uint16_t, uint8_t);

struct SREG {
    union {
        uint8_t bits;
        struct {
            uint8_t C :1;
            uint8_t Z :1;
            uint8_t N :1;
            uint8_t V :1;
            uint8_t S :1;
            uint8_t H :1;
            uint8_t T :1;
            uint8_t I :1;
        };
    };
};

struct SRAM {
    union {
        uint8_t bytes[SRAM_SIZE_BYTES];
        uint8_t regs[REGS_SIZE_BYTES];
    };
};

struct FLASH {
    union {
        uint8_t bytes[FLASH_SIZE_BYTES];
        uint16_t words[FLASH_SIZE_WORDS];
    };
};

struct AVR {
    uint16_t pc;
    uint64_t cycle;
    struct SRAM sram;
    struct FLASH flash;
    struct SREG sreg;

    avr_regs_func read_handler[REGS_SIZE_BYTES];
    avr_regs_func write_handler[REGS_SIZE_BYTES];
};

extern struct AVR avr;

void avr_init();
void avr_reset();
void avr_run();

uint8_t avr_read_byte(uint16_t addr);
void avr_write_byte(uint16_t addr, uint8_t data);
uint16_t avr_read_word(uint16_t addr);
void avr_write_word(uint16_t addr, uint16_t data);

uint8_t avr_pop_byte();
void avr_push_byte(uint8_t data);
uint16_t avr_pop_word();
void avr_push_word(uint16_t data);

void avr_unimplemented(const char *s);
void avr_illegalinst(uint16_t inst);
void avr_break();

#endif
