// main.cc

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <functional>

#include "avr.hh"

void usage(const char *fn)
{
    fprintf(stderr, "usage: %s [-t type] file\n", fn);
    fprintf(stderr, "       %s -h\n", fn);
}

int main(int argc, char *argv[])
{
    char ch;

    while((ch = getopt(argc, argv, "t:h")) != -1)
    {
        switch(ch)
        {
        case 't':
            break;
        case '?':
            fprintf(stderr, "\n");
        case 'h':
            usage(argv[0]);
            exit(1);
        }
    }
    if(argv[optind] == NULL)
    {
        usage(argv[0]);
        exit(1);
    }

    return 0;
}
