// usart.cc

#include <sys/time.h>
#include <unistd.h>

#include "usart.hh"

USART::USART(AVR *_avr, int _ifd, int _ofd, uint16_t _UDR, uint16_t _UCSRA, uint16_t _UCSRB, uint16_t _UCSRC, int _RXC, int _DRE, int _TXC)
    : avr(_avr), ifd(_ifd), ofd(_ofd), UDR(_UDR), UCSRA(_UCSRA), UCSRB(_UCSRB), UCSRC(_UCSRC), RXC(_RXC), DRE(_DRE), TXC(_TXC)
{
}

USART::~USART()
{
}

void USART::initialize()
{
    ucsra = USART_UCSRA_UDRE;

    avr->register_handler(UDR,
        [this](AVR *avr, uint16_t reg, uint8_t data)
        {
            if((ucsrb & USART_UCSRB_RXEN) && (ucsra & USART_UCSRA_RXC))
            {
                data = rdr;
                ucsra &= ~USART_UCSRA_RXC;
            }
            return data;
        },
        [this](AVR *avr, uint16_t reg, uint8_t data)
        {
            if((ucsrb & USART_UCSRB_TXEN) && (ucsra & USART_UCSRA_UDRE))
            {
                tdr = data;
                ucsra &= ~USART_UCSRA_TXC;
                ucsra &= ~USART_UCSRA_UDRE;
            }
            return data;
        });
    avr->register_handler(UCSRA,
        [this](AVR *avr, uint16_t reg, uint8_t data)
        {
            return data | ucsra;
        },
        [this](AVR *avr, uint16_t reg, uint8_t data)
        {
            return data & 0x1f;
        });
    avr->register_handler(UCSRB,
        [this](AVR *avr, uint16_t reg, uint8_t data)
        {
            return data = ucsrb;
        },
        [this](AVR *avr, uint16_t reg, uint8_t data)
        {
            return ucsrb = data;
        });
}

void USART::process()
{
    fd_set readfds, writefds;
    struct timeval tv = {0, 0};

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(ifd, &readfds);
    FD_SET(ofd, &writefds);

    if(0 < select((ifd < ofd ? ofd : ifd) + 1, &readfds, &writefds, NULL, &tv))
    {
        if((ucsrb & USART_UCSRB_RXEN) && (ucsra & USART_UCSRA_RXC) == 0 && FD_ISSET(ifd, &readfds))
        {
            if(read(ifd, &rdr, 1) == 1)
            {
                ucsra |= USART_UCSRA_RXC;
                if(ucsrb & USART_UCSRB_RXCIE)
                {
                    avr->raise_irq(RXC);
                }
            }
        }
        if((ucsrb & USART_UCSRB_TXEN) && (ucsra & USART_UCSRA_UDRE) == 0 && FD_ISSET(ofd, &writefds))
        {
            if(write(ofd, &tdr, 1) == 1)
            {
                ucsra |= USART_UCSRA_TXC;
                if(ucsrb & USART_UCSRB_TXCIE)
                {
                    avr->raise_irq(TXC);
                }
                ucsra |= USART_UCSRA_UDRE;
                if(ucsrb & USART_UCSRB_UDRIE)
                {
                    avr->raise_irq(DRE);
                }
            }
        }
    }
}
