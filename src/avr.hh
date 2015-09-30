// avr.hh

#ifndef AVRE_AVR_HH
#define AVRE_AVR_HH

#include <cstdint>
#include <functional>

#include "module.hh"
#include "instruction.hh"

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

#define IRQ_COUNT (27)

struct SREG
{
    union
    {
        uint8_t bits;
        struct
        {
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

struct SRAM
{
    union
    {
        uint8_t bytes[SRAM_SIZE_BYTES];
        uint8_t regs[REGS_SIZE_BYTES];
    };
};

struct FLASH
{
    union
    {
        uint8_t bytes[FLASH_SIZE_BYTES];
        uint16_t words[FLASH_SIZE_WORDS];
    };
};

class AVR : public Module
{
protected:
    typedef std::function<int(AVR *, uint16_t)> instruction;
    typedef std::function<uint8_t(AVR *, uint16_t, uint8_t)> access_handler;

    uint32_t irq;
    uint64_t cycle;

    access_handler read_handler[REGS_SIZE_BYTES];
    access_handler write_handler[REGS_SIZE_BYTES];

    static instruction instructions[INSTRUCTION_SPACE];

    void load_elf(const char *fn);
    void load_ihex(const char *fn);
    void load_bin(const char *fn);

public:
    uint16_t pc;
    struct SRAM sram;
    struct FLASH flash;
    struct SREG sreg;

    AVR(const char *fn, const char *tp);
    virtual ~AVR();

    virtual void initialize();
    virtual void process();

    void reset();
    void raise_irq(int num);
    void register_handler(uint16_t reg, access_handler read, access_handler write);

    uint8_t read_byte(uint16_t addr);
    void write_byte(uint16_t addr, uint8_t data);
    uint16_t read_word(uint16_t addr);
    void write_word(uint16_t addr, uint16_t data);

    uint8_t pop_byte();
    void push_byte(uint8_t data);
    uint16_t pop_word();
    void push_word(uint16_t data);

    void unimplemented(const char *s);
    void illegalinst(uint16_t inst);
};

#endif
