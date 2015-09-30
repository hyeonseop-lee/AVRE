// main.cc

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <functional>

#include "avr.hh"
#include "usart.hh"

void usage(const char *fn)
{
    fprintf(stderr, "usage: %s [-t type] file\n", fn);
    fprintf(stderr, "       %s -h\n", fn);
}

int main(int argc, char *argv[])
{
    AVR *avr;
    USART *usart0, *usart1;
    const char *type = NULL, *file = NULL;
    char ch;

    while((ch = getopt(argc, argv, "t:h")) != -1)
    {
        switch(ch)
        {
        case 't':
            type = optarg;
            break;
        case 'h':
        case '?':
            break;
        }
    }
    file = argv[optind];

    if(file == NULL || type == NULL)
    {
        usage(argv[0]);
        exit(1);
    }

    avr = new AVR(file, type);
    usart0 = new USART(avr, 3, 4, 0x2c, 0x2b, 0x2a, 0x95, 0x12, 0x13, 0x14);
    usart1 = new USART(avr, 5, 6, 0x9c, 0x9b, 0x9a, 0x9d, 0x1e, 0x1f, 0x20);

    avr->initialize();
    usart0->initialize();
    usart1->initialize();

    while(true)
    {
        avr->process();
        usart0->process();
        usart1->process();
    }

    return 0;
}
