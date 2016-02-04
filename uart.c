#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define BAUD_TOL 2
#define BAUD 115200
#include <util/setbaud.h>

#define BUFSIZE 1024

static uint8_t rx_buf[BUFSIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

static uint8_t tx_buf[BUFSIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;

ISR(USART0_RX_vect)
{
    uint16_t head = rx_head;
    uint16_t tail = rx_tail;

    while (bit_is_set(UCSR0A, RXC0))
    {
        uint8_t data = UDR0;

        if (((head + 1) % BUFSIZE) != tail)
        {
            rx_buf[head] = data;
            head = (head + 1) % BUFSIZE;
        }
    }

    rx_head = head;
}

ISR(USART0_UDRE_vect)
{
    uint16_t head = tx_head;
    uint16_t tail = tx_tail;

    while (bit_is_set(UCSR0A, UDRE0))
    {
        if (head != tail)
        {
            UDR0 = tx_buf[tail];
            tail = (tail + 1) % BUFSIZE;
        }
        else
        {
            UCSR0B &= ~_BV(UDRIE0);
            break;
        }
    }

    tx_tail = tail;
}

void uart_init(void)
{
    UBRR0 = UBRR_VALUE;
#if USE_2X
    UCSR0A = U2X0;
#else
    UCSR0A = 0;
#endif
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
    UCSR0B = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0);
}

void uart_putc(uint8_t data)
{
    uint16_t head = tx_head;

    NONATOMIC_BLOCK(NONATOMIC_RESTORESTATE)
    {
        uint16_t tail;

        do
        {
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
                tail = tx_tail;
            }
        }
        while (((head + 1) % BUFSIZE) == tail);
    }

    tx_buf[head] = data;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        tx_head = (head + 1) % BUFSIZE;
        UCSR0B |= _BV(UDRIE0);
    }
}

uint8_t uart_getc(void)
{
    uint16_t tail = rx_tail;
    uint8_t data;

    NONATOMIC_BLOCK(NONATOMIC_RESTORESTATE)
    {
        uint16_t head;

        do
        {
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
                head = rx_head;
            }
        }
        while (head == tail);
    }

    data = rx_buf[tail];

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        rx_tail = (tail + 1) % BUFSIZE;
    }

    return data;
}

uint16_t uart_bytes_available(void)
{
    uint16_t num_bytes;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        num_bytes = rx_head - rx_tail;
    }

    return (num_bytes % BUFSIZE);
}
