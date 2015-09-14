// loader.cc

#include <cstdio>
#include <cstdlib>

#include "avr.hh"

void AVR::load_elf(const char *fn)
{
}

void AVR::load_ihex(const char *fn)
{
    unsigned int size, addr, type, byte, checksum;
    int i;
    char *ptr;
    char tmp[8];
    FILE *f;

    f = fopen(fn, "r");
    if(f == NULL)
    {
        perror(fn);
        exit(1);
    }
    while(1)
    {
        while(!feof(f) && fgetc(f) != ':');
        if(fread(tmp, 8, 1, f) == 0)
        {
            perror(fn);
            exit(1);
        }
        if(tmp[6] != '0' || (tmp[7] != '0' && tmp[7] != '1'))
        {
            fprintf(stderr, "%s: illegal ihex file\n", fn);
            exit(1);
        }
        if(sscanf(tmp, "%02x%04x%02x", &size, &addr, &type) != 3)
        {
            fprintf(stderr, "%s: illegal ihex file\n", fn);
            exit(1);
        }
        if(FLASH_SIZE_BYTES <= addr + size)
        {
            fprintf(stderr, "%s: address out of range\n", fn);
            exit(1);
        }
        checksum = size + addr + (addr >> 8) + type;
        ptr = (char *)malloc(size << 1);
        if(size && fread(ptr, size << 1, 1, f) == 0)
        {
            perror(fn);
            exit(1);
        }
        for(i = 0; i < (int)size; i++)
        {
            if(sscanf(ptr + (i << 1), "%02x", &byte) != 1)
            {
                fprintf(stderr, "%s: illegal ihex file\n", fn);
                exit(1);
            }
            checksum += byte;
            if(tmp[7] == '0')
            {
                flash.bytes[addr + i] = byte;
            }
        }
        free(ptr);
        if(fread(tmp, 2, 1, f) == 0)
        {
            perror(fn);
            exit(1);
        }
        if(sscanf(tmp, "%02x", &byte) != 1)
        {
            fprintf(stderr, "%s: illegal ihex file\n", fn);
            exit(1);
        }
        if((checksum + byte) & 0xff)
        {
            fprintf(stderr, "%s: wrong ihex checksum\n", fn);
            exit(1);
        }
        if(tmp[7] == '1')
        {
            break;
        }
    }
}

void AVR::load_bin(const char *fn)
{
    unsigned int s, t;
    FILE *f;

    f = fopen(fn, "r");
    if(f == NULL)
    {
        perror(fn);
        exit(1);
    }
    s = 0;
    for(s = 0; s < FLASH_SIZE_BYTES && !feof(f); )
    {
        if((t = fread(flash.bytes + s, 1, FLASH_SIZE_BYTES - s, f)) == 0)
        {
            perror(fn);
            exit(1);
        }
        s += t;
    }
    if(!feof(f))
    {
        fprintf(stderr, "%s: address out of range\n", fn);
        exit(1);
    }
}
