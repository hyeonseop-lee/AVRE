// inst.h

#ifndef AVRE_INST_H
#define AVRE_INST_H

#include <stdint.h>

#define INST_COUNT (0x10000)

typedef void (*inst_func)(uint16_t);

extern inst_func inst_handler[INST_COUNT];

#endif
