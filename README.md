# AVRE
AVR Emulator for Reverse Engineering purpose

## Dependencies

- [Make](https://www.gnu.org/software/make/)
- [G++](https://gcc.gnu.org/) with C++11 supporting

## Build

	make

## Usage

	build/avre [-t type] file

#### Supported types
- ihex : Intel HEX 
- bin : raw binary

You can use avr-objcopy to convert AVR ELF to Intel HEX.

#### USART I/O
- USART0 TX / RX : File Descriptor 3 / 4
- USART1 TX / RX : File Descriptor 5 / 6

#### Example

	build/avre -t ihex program.hex 3<&0 4<&1

## License
Using emulator codes from [emulino](https://github.com/ghewgill/emulino), which was licensed under GNU GPLv3.

AVRE is licensed under GNU GPLv3.

