#pragma once
/*
 * simpleuart.h
 *
 * Created on: March 24, 2018
 *     Author: Chris van Zomeren
 *
 * A simple UART driver with blocking transmission and interrupt-driven reception.
 *
 * Devices are initialized with uart_init(), which takes a device configuration
 * and returns an object representing the configured hardware device.
 * After initializing, the device is disabled by default.
 *
 * The device may be enabled using uart_enable() and disabled using uart_disable().
 *
 * Data is transmitted using one of the uart_send_* functions. These are:
 *  - uart_send_byte() for a single character
 *  - uart_send_bytes() for a known number of characters
 *  - uart_send_string() for a string or array of characters terminated by '\0'
 *
 * Data is received asynchronously, and handled by a user-provided handler,
 * set using uart_set_receive_handler().
 * This handler consists of a function, and a "context" pointer used by the function to
 * store data between calls. The context may point to an arbitrary data structure used
 * by the handler function, or it may be NULL if it is not used by the handler function.
 * When called, the handler function is given two arguments:
 *  - the user-provided "context" pointer
 *  - the character received.
 *
 * Some improvements could be made, such as (in no particular order):
 *  - making the transmission also interrupt-driven, to avoid blocking application code
 *  - verifying baud settings when initializing a module
 *  - implementing some of the advanced features supported by the hardware UART module
 *  - implementing DMA send/receive
 *  - detecting changes to SMCLK and adjusting baud settings appropriately
 *  - providing some commonly-used receive callbacks (e.g. echo/forwarding, or buffered receive)
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


// This function should be provided by the user of this library.
// TODO: This is only used in uart_disable, as a quick hack to avoid losing the last transmitted character.
extern void delay_ms(uint32_t n);


// Flag constants for struct uart_config

#define UART_FLAG_PARITY_NONE 0               // No parity bit (default)
#define UART_FLAG_PARITY_ODD UCPEN            // Odd parity
#define UART_FLAG_PARITY_EVEN (UCPEN | UCPAR) // Even parity

#define UART_FLAG_LSB_FIRST 0     // Least-significant-bit first (default)
#define UART_FLAG_MSB_FIRST UCMSB // Most-significant-bit first

#define UART_FLAG_8BIT_CHAR 0      // Eight-bit characters (default)
#define UART_FLAG_7BIT_CHAR UC7BIT // Seven-bit characters

#define UART_FLAG_ONE_STOP_BIT 0      // Send one stop bit after transmission (default)
#define UART_FLAG_TWO_STOP_BITS UCSPB // Send two stop bits after transmission



struct _uart_interface;

// Represents a ready-to-use UART interface. Obtained by calling uart_init().
typedef struct _uart_interface *UartInterface;

// Used to supply the callback function, which is called every time the interface
// receives a character.
//
// callback is the callback which is called on every character.
//
// context is an optional pointer to arbitrary data needed by the callback.
// If your callback does not need to maintain any state between calls, you
// probably don't need this.
struct uart_receive_callback
{
    void (*callback)(void *context, char data);
    void *context;
};

// Configuration used to initialize a UART interface.
//
// id is the ID of the interface. On the MSP432, there are 4 interfaces,
// EUSCI_A0 -- EUSCI_A3, so ID is a number between 0 and 3.
// Each interface has its own associated I/O pins, which are documented
// in the hardware reference manual.
//
// baud_rate is the desired baud rate. Some baud rates may not be possible
// with a given clock configuration.
//
// flags is some combination of the constants named UART_FLAG_*.
// These are used to set parity, bit order, character size, and the number of stop bits.
//
struct uart_config
{
    int id;             // hardware interface ID. On the MSP432, there are 4 of these, eUSCI0 (0) through eUSCI3 (3)
    uint32_t baud_rate; // baud rate
    uint16_t flags;     // some combination of UART_FLAG_*, or 0 to use the hardware defaults.
};

// Initializes a UART interface with the given config, and returns a pointer to the
// initialized interface.
UartInterface uart_init(
    struct uart_config config);

// Sets the function called when the given interface receives a character.
void uart_set_receive_handler(
    UartInterface iface,
    struct uart_receive_callback callback);

// Enables the given interface, allowing it to send and receive.
void uart_enable(
    UartInterface iface);

// Disables the given interface, while leaving it initialized.
void uart_disable(
    UartInterface iface);

// Sends a single character over the given interface.
void uart_send_byte(
    UartInterface iface,
    char msg);

// Sends a fixed number of characters over the given interface.
void uart_send_bytes(
    UartInterface iface,
    size_t msg_len,
    char const *msg);

// Sends a '\0'-terminated string over the given interface.
void uart_send_string(
    UartInterface iface,
    char const *msg);
