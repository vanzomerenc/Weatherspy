// I2C.c
// Runs on MSP432
// Provide a function that initializes, sends, and receives
// the eUSCI1B module interfaced with A BME280 pressure sensor
//
// based on Mazidi et.al.
// see http://www.microdigitaled.com/ARM/MSP432_ARM/Code/Ver1/red/Chapter9
//
// modified by RWB 2/9/2018

#include <drv/i2c/i2c.h>
#include "driverlib.h"
#include <stdint.h>
#include "msp.h"


/* configure UCB1 as I2C */
void I2C_Init(void) {

    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P6, GPIO_PIN4 | GPIO_PIN5);
    UCB1CTLW0 = 0x0001;  // hold the eUSCI module in reset mode
    delay_ms(100);
    // configure UCB1CTLW0 for:
    // bit15      UCA10 = 0; own address is 7-bit address
    // bit14      UCSLA10 = 0; address slave with 7-bit address
    // bit13      UCMM = 0; single master environment
    // bit12      reserved
    // bit11      UCMST = 1; master mode
    // bits10-9   UCMODEx = 3; I2C mode
    // bit8       UCSYNC = 1; synchronous mode
    // bits7-6    UCSSELx = 2; eUSCI clock SMCLK
    // bit5       UCTXACK = X; transmit ACK condition in slave mode
    // bit4       UCTR = X; transmitter/receiver
    // bit3       UCTXNACK = X; transmit negative acknowledge in slave mode
    // bit2       UCTXSTP = X; transmit stop condition in master mode
    // bit1       UCTXSTT = X; transmit start condition in master mode
    // bit0       UCSWRST = 1; reset enabled
    UCB1CTLW0 = 0x0F81;

    // set the baud rate for the eUSCI which gets its clock from SMCLK
    // Clock_Init48MHz() from ClockSystem.c sets SMCLK = HFXTCLK/4 = 12 MHz
    // if the SMCLK is set to 12 MHz, divide by 120 for 100 kHz baud clock
    UCB1BRW = 480;
    P6SEL0 |= 0x30;
    P6SEL1 &= ~0x30;                   // configure P6.4 and P6.5 as primary module function
    UCB1CTLW0 &= ~0x0001;              // enable eUSCI module
    UCB1IE = 0x0000;                   // disable interrupts
}

/*
 *  subroutine to write bytes to BME280 registers
 */

int I2C_WRITE_STRING(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t byteCount)
{
    if (byteCount <= 0)
        return -1;              /* no write was performed */

    int timeout;

    EUSCI_B1->I2CSA = dev_addr;      /* setup slave address */
    EUSCI_B1->CTLW0 |= 0x0010;       /* enable transmitter */
    EUSCI_B1->CTLW0 |= 0x0002;       /* generate START and send slave address */

    timeout = I2C_TIMEOUT;
    while((EUSCI_B1->CTLW0 & 2) && timeout--);    /* wait until slave address is sent */
    if(!timeout) goto failed_transmission;

    EUSCI_B1->TXBUF = reg_addr;       /* send memory address to slave */

    /* send data one byte at a time */
    do {
        timeout = I2C_TIMEOUT;
        while(!(EUSCI_B1->IFG & 2) && timeout--);     /* wait till it's ready to transmit */
        if(!timeout) goto failed_transmission;

        EUSCI_B1->TXBUF = *reg_data++;   /* send data to slave */
        byteCount--;
     } while (byteCount > 0);

    timeout = I2C_TIMEOUT;
    while(!(EUSCI_B1->IFG & 2) && timeout--);      /* wait till last transmit is done */
    if(!timeout) goto failed_transmission;

    EUSCI_B1->CTLW0 |= 0x0004;        /* send STOP */

    timeout = I2C_TIMEOUT;
    while(EUSCI_B1->CTLW0 & 4 && timeout--);      /* wait until STOP is sent */
    if(!timeout) goto failed_transmission;

    return 0;                   /* no error */

failed_transmission:
    EUSCI_B1->CTLW0 |= 0x0004;        /* send STOP */
    return -1;
}

/*
 *  subroutine to read bytes from BME280 registers
 */

int I2C_WRITE_READ_STRING(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t byteCount)
{
    if (byteCount <= 0)
        return -1;              /* no read was performed */

    int timeout;

    EUSCI_B1->I2CSA = dev_addr;     /* setup slave address */
    EUSCI_B1->CTLW0 |= 0x0010;      /* enable transmitter */
    EUSCI_B1->CTLW0 |= 0x0002;      /* generate START and send slave address */

    timeout = I2C_TIMEOUT;
    while((EUSCI_B1->CTLW0 & 2) && timeout--);   /* wait until slave address is sent */
    if(!timeout) goto failed_transmission;

    EUSCI_B1->TXBUF = reg_addr;     /* send memory address to slave */

    timeout = I2C_TIMEOUT;
    while(!(EUSCI_B1->IFG & 2) && timeout--);    /* wait till last transmit is done */
    if(!timeout) goto failed_transmission;

    EUSCI_B1->CTLW0 &= ~0x0010;     /* enable receiver */
    EUSCI_B1->CTLW0 |= 0x0002;      /* generate RESTART and send slave address */

    timeout = I2C_TIMEOUT;
    while(EUSCI_B1->CTLW0 & 2 && timeout--);     /* wait till RESTART is finished */
    if(!timeout) goto failed_transmission;

    /* receive data one byte at a time */
    do {
        if (byteCount == 1)     /* when only one byte of data is left */
            EUSCI_B1->CTLW0 |= 0x0004; /* setup to send STOP after the last byte is received */

        timeout = I2C_TIMEOUT;
        while(!(EUSCI_B1->IFG & 1) && timeout--);    /* wait till data is received */
        if(!timeout) goto failed_transmission;

        *reg_data++ = EUSCI_B1->RXBUF;  /* read the received data */
        byteCount--;
    } while (byteCount);

    timeout = I2C_TIMEOUT;
    while(EUSCI_B1->CTLW0 & 4 && timeout--) ;      /* wait until STOP is sent */
    if(!timeout) goto failed_transmission;

    return 0;                   /* no error */

failed_transmission:
    EUSCI_B1->CTLW0 |= 0x0004;        /* send STOP */
    return -1;
}
