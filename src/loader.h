// loader.h

#ifndef AVRE_LOADER_H
#define AVRE_LOADER_H

#include "avr.h"

void load_elf(const char *fn);
void load_ihex(const char *fn);
void load_bin(const char *fn);

#endif
