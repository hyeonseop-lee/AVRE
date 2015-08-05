// main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "avr.h"
#include "loader.h"

void usage(const char *fn) {
    fprintf(stderr, "usage: %s [-t type] file\n", fn);
    fprintf(stderr, "       %s -h\n", fn);
}

int main(int argc, char *argv[]) {
    void (*loader)(const char *);
    char ch;

    loader = load_elf;
    while((ch = getopt(argc, argv, "t:h")) != -1) {
        switch(ch) {
        case 't':
            if(strcasecmp(optarg, "elf") == 0) {
                loader = load_elf;
            }
            else if(strcasecmp(optarg, "ihex") == 0) {
                loader = load_ihex;
            }
            else if(strcasecmp(optarg, "bin") == 0) {
                loader = load_bin;
            }
            else {
                fprintf(stderr, "%s: unknown file loader -- '%s'\n\n", argv[0], optarg);
                usage(argv[0]);
                exit(1);
            }
            break;
        case '?':
            fprintf(stderr, "\n");
        case 'h':
            usage(argv[0]);
            exit(1);
        }
    }
    if(argv[optind] == NULL) {
        usage(argv[0]);
        exit(1);
    }

    loader(argv[optind]);

    return 0;
}
