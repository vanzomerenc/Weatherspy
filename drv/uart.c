/*
 * uart.c
 *
 *  Created on: Mar 24, 2018
 *      Author: Chris van Zomeren
 */

#include <msp432p401r.h>
#include <cs.h>
#include <interrupt.h>
#include <stdbool.h>
#include <stdint.h>

#include <drv/uart.h>

#define UART_FLAGS_MASK (UCPEN | UCPAR | UCMSB | UC7BIT | UCSPB)

#define UART_BAUD_MODULATION_TABLE_SIZE 36
static uint16_t _baud_calc_frac_part[UART_BAUD_MODULATION_TABLE_SIZE];
static uint8_t _baud_calc_mod_pattern[UART_BAUD_MODULATION_TABLE_SIZE];



// It's probably worth providing an overview of the interrupt handling here.
//
// When a UartInterface is initialized, one of these is set as its interrupt handler.
// They're numbered based on which hardware module is being used (out of the 4 available).
// Each module has its own handler and its own struct _uart_interface containing
// all the information for that interface.
//
// Each of these handlers is just a stub which delegates to
//     _eusci_common_uart_interrupt_handler,
// providing that common handler with a pointer to the interface it corresponds to.
//
// The common handler reads from the receive register for the hardware module, then
// calls the user-supplied callback for the interface, supplying it a user-supplied
// "context" pointer and the received byte.
//
// This makes it possible to perform arbitrary processing of the received data in the
// user-supplied callback, rather than requiring the user to write their own complete
// interrupt handler.
//
// Obviously, the callback should still be kept relatively simple, since it is still
// being run from an interrupt handler.
//
static void _eusci_a0_uart_interrupt_handler(void);
static void _eusci_a1_uart_interrupt_handler(void);
static void _eusci_a2_uart_interrupt_handler(void);
static void _eusci_a3_uart_interrupt_handler(void);

struct _uart_interface
{
    struct uart_receive_callback _rx_callback; // User function called when a byte is received
    EUSCI_A_Type *_module;                     // raw hardware module
    uint32_t _interrupt_id;                    // DriverLib interrupt ID
    void (*_interrupt_handler)(void);          // interrupt handler for module
    DIO_PORT_Interruptable_Type *_port;        // raw hardware port for module
    int16_t _rx_pin;                           // bitmask for receive pin
    int16_t _tx_pin;                           // bitmask for transmit pin
};

static struct _uart_interface _uart_interface[4] = {
    (struct _uart_interface) { {NULL, NULL}, EUSCI_A0, INT_EUSCIA0, &_eusci_a0_uart_interrupt_handler, PA, 1 << 3,       1 << 2 },
    (struct _uart_interface) { {NULL, NULL}, EUSCI_A1, INT_EUSCIA1, &_eusci_a1_uart_interrupt_handler, PA, 1 << (3 + 8), 1 << (2 + 8) },
    (struct _uart_interface) { {NULL, NULL}, EUSCI_A2, INT_EUSCIA2, &_eusci_a2_uart_interrupt_handler, PB, 1 << 3,       1 << 2 },
    (struct _uart_interface) { {NULL, NULL}, EUSCI_A3, INT_EUSCIA3, &_eusci_a3_uart_interrupt_handler, PE, 1 << 7,       1 << 6 }
};



void uart_send_byte(UartInterface iface, char msg)
{
    while(!(iface->_module->IFG & UCTXIFG))
    {
        // wait for last transmit to finish
    }
    iface->_module->TXBUF = msg;
}

void uart_send_bytes(UartInterface iface, size_t msg_len, char const *msg)
{
    for(size_t i = 0; i < msg_len; i++)
    {
        uart_send_byte(iface, msg[i]);
    }
}

void uart_send_string(UartInterface iface, char const *msg)
{
    for(size_t i = 0; msg[i] != '\0'; i++)
    {
        uart_send_byte(iface, msg[i]);
    }
}



// See the comment on interrupt handling near the top of this file.
static void _eusci_common_uart_interrupt_handler(struct _uart_interface *iface)
{
    if(iface->_module->IFG & UCRXIFG)
    {
        char received = iface->_module->RXBUF;
        if(iface->_rx_callback.callback != NULL)
        {
            iface->_rx_callback.callback(iface->_rx_callback.context, received);
        }
    }
}



struct _uart_baud_register_config
{
    uint32_t divider;
    bool use_16x_oversampling;
    uint8_t modulation_pattern;
    uint8_t oversampling_pattern;
};

// This calculation is taken from the MSP432P4xx technical reference manual, Rev. H,
// sec. 24.3.10 Setting a Baud rate,
// modified to use integer math only.
//
// Even though we're doing this automatically, it's probably still
// a good idea to run through TI's calculator and find out if the
// SMCLK we're using can support the baud rate we want.
// This function does no error analysis, and will happily give us
// garbage if it's asked for something unreasonable.
//
static struct _uart_baud_register_config _calculate_settings_for_baud_rate(uint32_t clock_rate, uint32_t baud_rate)
{
    uint32_t N = clock_rate / baud_rate;
    uint16_t frac_N = ((clock_rate % baud_rate) * (1 << 16)) / baud_rate;

    struct _uart_baud_register_config result = {0};

    if(N <= 16)
    {
        // Use low-frequency baud rate generation
        result.use_16x_oversampling = false;
        result.divider = N;
        result.oversampling_pattern = 0;
    }
    else
    {
        // Use high-frequency baud rate generation
        result.use_16x_oversampling = true;
        result.divider = N / 16;
        result.oversampling_pattern = N % 16;
    }

    result.modulation_pattern = 0x00;
    for(int i = 1; i < UART_BAUD_MODULATION_TABLE_SIZE; i++)
    {
        if(_baud_calc_frac_part[i] > frac_N)
        {
            result.modulation_pattern = _baud_calc_mod_pattern[i-1];
            break;
        }
    }

    return result;
}



static void _uart_pins_init(struct _uart_interface *iface)
{
    DIO_PORT_Interruptable_Type *port = iface->_port;
    uint16_t tx_pin = iface->_tx_pin;
    uint16_t rx_pin = iface->_rx_pin;
    uint16_t both_pins = tx_pin | rx_pin;

    // disable interrupts
    port->IE &= ~both_pins;

    // clear interrupt flags
    port->IFG &= ~both_pins;

    // set pin directions
    port->DIR |= tx_pin;
    port->DIR &= ~rx_pin;

    // set low drive strength
    port->DS &= ~both_pins;

    // disable pull resistors
    port->REN &= ~both_pins;

    // set pins to UART mode
    port->SEL1 &= ~both_pins;
    port->SEL0 |= both_pins; // UART is primary function on all used pins
}



UartInterface uart_init(struct uart_config config)
{
    struct _uart_interface *iface = &_uart_interface[config.id];

    uart_disable(iface); // before doing anything else

    uint32_t clock_rate = CS_getSMCLK(); // using driverlib function since this is complicated

    struct _uart_baud_register_config baud_settings
        = _calculate_settings_for_baud_rate(clock_rate, config.baud_rate);

    _uart_pins_init(iface);

    EUSCI_A_Type *mod = iface->_module;

    // keep disabled, but reset the rest of the control register
    mod->CTLW0 = UCSWRST;
    mod->CTLW1 = 0;

    mod->CTLW0 |= UCSSEL0 | UCSSEL1;  // set clock source to SMCLK

    mod->CTLW0 |= config.flags & UART_FLAGS_MASK; // set flags configurable by caller

    mod->BRW = baud_settings.divider; // set divider

    // set modulation
    mod->MCTLW = 0;
    mod->MCTLW |= (baud_settings.modulation_pattern << 8) & 0xFF00;
    mod->MCTLW |= (baud_settings.oversampling_pattern << 4) & 0x00F0;
    mod->MCTLW |= baud_settings.use_16x_oversampling? 1 : 0;


    // register and enable module-wide interrupt
    Interrupt_registerInterrupt(iface->_interrupt_id, iface->_interrupt_handler);
    Interrupt_enableInterrupt(iface->_interrupt_id);

    // remove any existing receive callback
    iface->_rx_callback.callback = NULL;
    iface->_rx_callback.context = NULL;

    return iface;
}

void uart_set_receive_handler(UartInterface iface, struct uart_receive_callback callback)
{
    iface->_rx_callback = callback;
}

void uart_enable(UartInterface iface)
{
    iface->_module->CTLW0 &= ~UCSWRST;

    iface->_module->IE = UCRXIE; // enable interrupt only for receive
}

void uart_disable(UartInterface iface)
{
    iface->_module->CTLW0 |= UCSWRST;
}





// Calculated using scripts/uart_divider_fractional_part_as_integers.apl
// based on fractional values from MSP432P4xx technical reference manual, Rev. H,
// table 24-4 UCBRSx Settings for Fractional Portion of N=f_BRCLK/Baud Rate
static uint16_t _baud_calc_frac_part[UART_BAUD_MODULATION_TABLE_SIZE] =
{
     0,  3466,  4685,  5472,  6560,  8205,  9371, 10944, 14070,
 14575, 16403, 19660, 21856, 23429, 24595, 26234, 28088, 28691,
 32781, 37453, 39341, 40986, 42152, 43692, 45881, 46838, 49171,
 51517, 52455, 54611, 55469, 56177, 57350, 59008, 60096, 60869
};


// Based on MSP432P4xx technical reference manual, Rev. H,
// table 24-4 UCBRSx Settings for Fractional Portion of N=f_BRCLK/Baud Rate
static uint8_t _baud_calc_mod_pattern[UART_BAUD_MODULATION_TABLE_SIZE] =
{
 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x11, 0x21,
 0x22, 0x44, 0x25, 0x49, 0x4A, 0x52, 0x92, 0x53, 0x55,
 0xAA, 0x6B, 0xAD, 0xB5, 0xB6, 0xD6, 0xB7, 0xBB, 0xDD,
 0xED, 0xEE, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE
};



static void _eusci_a0_uart_interrupt_handler(void)
{
    _eusci_common_uart_interrupt_handler(&_uart_interface[0]);
}

static void _eusci_a1_uart_interrupt_handler(void)
{
    _eusci_common_uart_interrupt_handler(&_uart_interface[1]);
}

static void _eusci_a2_uart_interrupt_handler(void)
{
    _eusci_common_uart_interrupt_handler(&_uart_interface[2]);
}

static void _eusci_a3_uart_interrupt_handler(void)
{
    _eusci_common_uart_interrupt_handler(&_uart_interface[3]);
}
