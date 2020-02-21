#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "uart.h"
#include <util/crc16.h>

#if defined(__AVR_AT90S2313__) || defined(__AVR_AT90S4414__) || defined(__AVR_AT90S8515__) || \
    defined(__AVR_AT90S4434__) || defined(__AVR_AT90S8535__) || \
    defined(__AVR_ATmega103__)
 /* old AVR classic or ATmega103 with one UART */
 #define UART0_RECEIVE_INTERRUPT   UART_RX_vect 
 #define UART0_TRANSMIT_INTERRUPT  UART_UDRE_vect
 #define UART0_STATUS      USR
 #define UART0_CONTROL     UCR
 #define UART0_DATA        UDR  
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRR
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
#elif defined(__AVR_AT90S2333__) || defined(__AVR_AT90S4433__)
 /* old AVR classic with one UART */
 #define UART0_RECEIVE_INTERRUPT   UART_RX_vect 
 #define UART0_TRANSMIT_INTERRUPT  UART_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_DATA        UDR 
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRR
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
#elif defined(__AVR_AT90PWM216__) || defined(__AVR_AT90PWM316__) 
 /* AT90PWN216/316 with one USART */
 #define UART0_RECEIVE_INTERRUPT   USART_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_CONTROLC    UCSRC
 #define UART0_DATA        UDR
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRRL
 #define UART0_UBRRH       UBRRH
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
 #define UART0_BIT_UCSZ0   UCSZ0
 #define UART0_BIT_UCSZ1   UCSZ1 
#elif defined(__AVR_ATmega8__) || defined(__AVR_ATmega8A__) || \
      defined(__AVR_ATmega16__) || defined(__AVR_ATmega16A__) || \
      defined(__AVR_ATmega32__) || defined(__AVR_ATmega32A__) || \
      defined(__AVR_ATmega323__)
 /* ATmega with one USART */
 #define UART0_RECEIVE_INTERRUPT   USART_RXC_vect
 #define UART0_TRANSMIT_INTERRUPT  USART_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_CONTROLC    UCSRC
 #define UART0_DATA        UDR
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRRL
 #define UART0_UBRRH       UBRRH
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
 #define UART0_BIT_UCSZ0   UCSZ0
 #define UART0_BIT_UCSZ1   UCSZ1
 #define UART0_BIT_URSEL   URSEL
#elif defined (__AVR_ATmega8515__) || defined(__AVR_ATmega8535__)
 #define UART0_RECEIVE_INTERRUPT   USART_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_CONTROLC    UCSRC 
 #define UART0_DATA        UDR
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRRL
 #define UART0_UBRRH       UBRRH
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
 #define UART0_BIT_UCSZ0   UCSZ0
 #define UART0_BIT_UCSZ1   UCSZ1
 #define UART0_BIT_URSEL   URSEL
#elif defined(__AVR_ATmega163__)
  /* ATmega163 with one UART */
 #define UART0_RECEIVE_INTERRUPT   UART_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  UART_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_DATA        UDR
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRR
 #define UART0_UBRRH       UBRRHI
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
#elif defined(__AVR_ATmega162__) 
 /* ATmega with two USART */
 #define ATMEGA_USART1
 #define UART0_RECEIVE_INTERRUPT   USART0_RXC_vect
 #define UART1_RECEIVE_INTERRUPT   USART1_RXC_vect
 #define UART0_TRANSMIT_INTERRUPT  USART0_UDRE_vect
 #define UART1_TRANSMIT_INTERRUPT  USART1_UDRE_vect
 #define UART0_STATUS      UCSR0A
 #define UART0_CONTROL     UCSR0B
 #define UART0_CONTROLC    UCSR0C
 #define UART0_DATA        UDR0
 #define UART0_UDRIE       UDRIE0
 #define UART0_UBRRL       UBRR0L
 #define UART0_UBRRH       UBRR0H
 #define UART0_BIT_URSEL   URSEL0
 #define UART0_BIT_U2X     U2X0
 #define UART0_BIT_RXCIE   RXCIE0
 #define UART0_BIT_RXEN    RXEN0
 #define UART0_BIT_TXEN    TXEN0
 #define UART0_BIT_UCSZ0   UCSZ00
 #define UART0_BIT_UCSZ1   UCSZ01
 #define UART1_STATUS      UCSR1A
 #define UART1_CONTROL     UCSR1B
 #define UART1_CONTROLC    UCSR1C
 #define UART1_DATA        UDR1
 #define UART1_UDRIE       UDRIE1
 #define UART1_UBRRL       UBRR1L
 #define UART1_UBRRH       UBRR1H
 #define UART1_BIT_URSEL   URSEL1
 #define UART1_BIT_U2X     U2X1
 #define UART1_BIT_RXCIE   RXCIE1
 #define UART1_BIT_RXEN    RXEN1
 #define UART1_BIT_TXEN    TXEN1
 #define UART1_BIT_UCSZ0   UCSZ10
 #define UART1_BIT_UCSZ1   UCSZ11 
#elif defined(__AVR_ATmega161__)
 /* ATmega with UART */
 #error "AVR ATmega161 currently not supported by this libaray !"
#elif defined(__AVR_ATmega169__)
 /* ATmega with one USART */
 #define UART0_RECEIVE_INTERRUPT   USART0_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART0_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_CONTROLC    UCSRC
 #define UART0_DATA        UDR
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRRL
 #define UART0_UBRRH       UBRRH
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
 #define UART0_BIT_UCSZ0   UCSZ0
 #define UART0_BIT_UCSZ1   UCSZ1
#elif defined(__AVR_ATmega48__) || defined(__AVR_ATmega48A__) || defined(__AVR_ATmega48P__) || defined(__AVR_ATmega48PA__) || defined(__AVR_ATmega48PB__) || \
      defined(__AVR_ATmega88__) || defined(__AVR_ATmega88A__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega88PA__) || defined(__AVR_ATmega88PB__) || \
      defined(__AVR_ATmega168__) || defined(__AVR_ATmega168A__)|| defined(__AVR_ATmega168P__)|| defined(__AVR_ATmega168PA__) || defined(__AVR_ATmega168PB__) || \
      defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) || \
      defined(__AVR_ATmega3250__) || defined(__AVR_ATmega3290__) ||defined(__AVR_ATmega6450__) || defined(__AVR_ATmega6490__)
 /* ATmega with one USART */
 #define UART0_RECEIVE_INTERRUPT   USART_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART_UDRE_vect
 #define UART0_STATUS      UCSR0A
 #define UART0_CONTROL     UCSR0B
 #define UART0_CONTROLC    UCSR0C
 #define UART0_DATA        UDR0
 #define UART0_UDRIE       UDRIE0
 #define UART0_UDRE        UDRE0
 #define UART0_UBRRL       UBRR0L
 #define UART0_UBRRH       UBRR0H
 #define UART0_BIT_U2X     U2X0
 #define UART0_BIT_RXCIE   RXCIE0
 #define UART0_BIT_RXEN    RXEN0
 #define UART0_BIT_TXEN    TXEN0
 #define UART0_BIT_UCSZ0   UCSZ00
 #define UART0_BIT_UCSZ1   UCSZ01
#elif defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny2313A__) || defined(__AVR_ATtiny4313__)
 /* ATtiny with one USART */
 #define UART0_RECEIVE_INTERRUPT   USART_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_CONTROLC    UCSRC
 #define UART0_DATA        UDR
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRRL
 #define UART0_UBRRH       UBRRH
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
 #define UART0_BIT_UCSZ0   UCSZ0
 #define UART0_BIT_UCSZ1   UCSZ1
#elif defined(__AVR_ATmega329__) || defined(__AVR_ATmega649__) || defined(__AVR_ATmega3290__) || defined(__AVR_ATmega6490__) ||\
      defined(__AVR_ATmega169A__) || defined(__AVR_ATmega169PA__) || \
      defined(__AVR_ATmega329A__) || defined(__AVR_ATmega329PA__) || defined(__AVR_ATmega3290A__) || defined(__AVR_ATmega3290PA__) || \
      defined(__AVR_ATmega649A__) || defined(__AVR_ATmega649P__) || defined(__AVR_ATmega6490A__) || defined(__AVR_ATmega6490P__) || \
      defined(__AVR_ATmega165__) || defined(__AVR_ATmega325__) || defined(__AVR_ATmega645__) || defined(__AVR_ATmega3250__) || defined(__AVR_ATmega6450__) || \
      defined(__AVR_ATmega165A__) || defined(__AVR_ATmega165PA__) || \
      defined(__AVR_ATmega325A__) || defined(__AVR_ATmega325PA__) || defined(__AVR_ATmega3250A__) || defined(__AVR_ATmega3250PA__) ||\
      defined(__AVR_ATmega645A__) || defined(__AVR_ATmega645PA__) || defined(__AVR_ATmega6450A__) || defined(__AVR_ATmega6450PA__) || \
      defined(__AVR_ATmega644__)
 /* ATmega with one USART */
 #define UART0_RECEIVE_INTERRUPT   USART0_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART0_UDRE_vect
 #define UART0_STATUS      UCSR0A
 #define UART0_CONTROL     UCSR0B
 #define UART0_CONTROLC    UCSR0C
 #define UART0_DATA        UDR0
 #define UART0_UDRIE       UDRIE0
 #define UART0_UBRRL       UBRR0L
 #define UART0_UBRRH       UBRR0H
 #define UART0_BIT_U2X     U2X0
 #define UART0_BIT_RXCIE   RXCIE0
 #define UART0_BIT_RXEN    RXEN0
 #define UART0_BIT_TXEN    TXEN0
 #define UART0_BIT_UCSZ0   UCSZ00
 #define UART0_BIT_UCSZ1   UCSZ01
#elif defined(__AVR_ATmega64__) || defined(__AVR_ATmega128__) || defined(__AVR_ATmega128A__) ||\
      defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) || \
      defined(__AVR_ATmega164P__) || defined(__AVR_ATmega324P__) || defined(__AVR_ATmega644P__) ||  \
      defined(__AVR_ATmega164A__) || defined(__AVR_ATmega164PA__) || defined(__AVR_ATmega324A__) || defined(__AVR_ATmega324PA__) || \
      defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644PA__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) ||\
      defined(__AVR_ATtiny1634__)
 /* ATmega with two USART */
 #define ATMEGA_USART1
 #define UART0_RECEIVE_INTERRUPT   USART0_RX_vect
 #define UART1_RECEIVE_INTERRUPT   USART1_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART0_UDRE_vect
 #define UART1_TRANSMIT_INTERRUPT  USART1_UDRE_vect
 #define UART0_STATUS      UCSR0A
 #define UART0_CONTROL     UCSR0B
 #define UART0_CONTROLC    UCSR0C  
 #define UART0_DATA        UDR0
 #define UART0_UDRIE       UDRIE0
 #define UART0_UBRRL       UBRR0L
 #define UART0_UBRRH       UBRR0H
 #define UART0_BIT_U2X     U2X0
 #define UART0_BIT_RXCIE   RXCIE0
 #define UART0_BIT_RXEN    RXEN0
 #define UART0_BIT_TXEN    TXEN0
 #define UART0_BIT_UCSZ0   UCSZ00
 #define UART0_BIT_UCSZ1   UCSZ01 
 #define UART1_STATUS      UCSR1A
 #define UART1_CONTROL     UCSR1B
 #define UART1_CONTROLC    UCSR1C  
 #define UART1_DATA        UDR1
 #define UART1_UDRIE       UDRIE1
 #define UART1_UBRRL       UBRR1L
 #define UART1_UBRRH       UBRR1H
 #define UART1_BIT_U2X     U2X1
 #define UART1_BIT_RXCIE   RXCIE1
 #define UART1_BIT_RXEN    RXEN1
 #define UART1_BIT_TXEN    TXEN1
 #define UART1_BIT_UCSZ0   UCSZ10
 #define UART1_BIT_UCSZ1   UCSZ11
#elif defined(__AVR_ATmega8U2__) || defined(__AVR_ATmega16U2__) || defined(__AVR_ATmega32U2__) || \
      defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__) || \
      defined(__AVR_AT90USB82__) || defined(__AVR_AT90USB162__) || \
      defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB647__) || defined(__AVR_AT90USB1287__)
 #define UART0_RECEIVE_INTERRUPT   USART1_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART1_UDRE_vect
 #define UART0_STATUS      UCSR1A
 #define UART0_CONTROL     UCSR1B
 #define UART0_CONTROLC    UCSR1C
 #define UART0_DATA        UDR1
 #define UART0_UDRIE       UDRIE1
 #define UART0_UDRE        UDRE1
 #define UART0_UBRRL       UBRR1L
 #define UART0_UBRRH       UBRR1H
 #define UART0_BIT_U2X     U2X1
 #define UART0_BIT_RXCIE   RXCIE1
 #define UART0_BIT_RXEN    RXEN1
 #define UART0_BIT_TXEN    TXEN1
 #define UART0_BIT_UCSZ0   UCSZ10
 #define UART0_BIT_UCSZ1   UCSZ11
#else
 #error "no UART definition for MCU available"
#endif

static int uart_putc(char c, FILE *stream);
static FILE mystdout = FDEV_SETUP_STREAM(uart_putc, NULL, _FDEV_SETUP_WRITE);

void uart_init(uint16_t baud_rate)
{
	// calculate baud rate
	baud_rate = (((F_CPU/(baud_rate*16UL))) - 1);

	// set rate
	UART0_UBRRH = (uint8_t)(baud_rate >> 8);
	UART0_UBRRL = (uint8_t) baud_rate;

	// Disable double transmission speed
	UART0_STATUS &= ~_BV(UART0_BIT_U2X);

	// Enable reciever, transmitter, and rx interrupts
	UART0_CONTROL |= _BV(UART0_BIT_RXEN) | _BV(UART0_BIT_TXEN);

	// Set frame format to 8 data
	UART0_CONTROLC |= _BV(UART0_BIT_UCSZ1) | _BV(UART0_BIT_UCSZ0);

	stdout = &mystdout;
}

static int
uart_putc(char c, FILE *stream)
{
	loop_until_bit_is_set(UART0_STATUS, UART0_UDRE);
	UART0_DATA = c;

	return 0;
}

void
uart_puts(const char* s)
{
	while(*s)
	{
		loop_until_bit_is_set(UART0_STATUS, UART0_UDRE);
		UART0_DATA = *s++;
	}
}
