// usart.hh

#ifndef AVRE_USART_HH
#define AVRE_USART_HH

#include <sstream>

#include "avr.hh"

#define USART_UCSRA_RXC   (0x80u)
#define USART_UCSRA_TXC   (0x40u)
#define USART_UCSRA_UDRE  (0x20u)
#define USART_UCSRB_RXCIE (0x80u)
#define USART_UCSRB_TXCIE (0x40u)
#define USART_UCSRB_UDRIE (0x20u)
#define USART_UCSRB_RXEN  (0x10u)
#define USART_UCSRB_TXEN  (0x08u)

class USART : public Module
{
protected:
    AVR *avr;
    int ifd, ofd;
    uint8_t rdr, tdr, ucsra, ucsrb;
    uint16_t UDR, UCSRA, UCSRB, UCSRC, RXC, DRE, TXC;

public:
    USART(AVR *_avr, int _ifd, int _ofd, uint16_t _UDR, uint16_t _UCSRA, uint16_t _UCSRB, uint16_t _UCSRC, int _RXC, int _DRE, int _TXC);
    virtual ~USART();

    virtual void initialize();
    virtual void process();
};

#endif
