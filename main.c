/*

 * Author: Sam
*******************************************************************************/
/* DriverLib Includes */
#include "driverlib.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <drv/uart.h>
#include <drv/uart_stdio_support.h>
#include <drv/timing.h>
#include <drv/esp8266.h>
#include <station.h>

#define READ_LINE_BUFSIZE (CHAR_MAX + 1)
#define READ_LINE_LAST_POSITION CHAR_MAX

struct uart_channel wifi_uart;
int main(void)
{
    /* Stop Watchdog  */
    MAP_WDT_A_holdTimer();

    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);

    init_clocks();
    uart_use_stdio_support();
    init_station_module();
    rtc_init();

    Interrupt_disableSleepOnIsrExit();
    Interrupt_enableMaster();

    uart_replace_standard_streams(
        (struct uart_config) {.id = 0, .baud_rate = 115200, .flags = 0},
        (struct uart_input_config) {.complete_lines = false, .echo = true},
        (struct uart_output_config) {});

    wifi_uart = uart_open(
            (struct uart_config) {.id = 2, .baud_rate = 115200, .flags = 0},
            (struct uart_input_config) {.complete_lines = false, .echo = false},
            (struct uart_output_config) {});

    puts("What up!\r\n");
    puts("Test\r\n");
    //puts("AT\r\n");
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);
    delay_ms(1000);

    //AtInterface wifi = at_init(wifi_uart.tx, wifi_uart.rx);
    //int err = at_check_alive(wifi);
    //printf("%d", err);

    delay_ms(50);
    char receive_buff[READ_LINE_BUFSIZE] = {'\0'};
    fseek(wifi_uart.rx, 0, SEEK_END);
    fputs("AT\r\n", wifi_uart.tx);
    delay_ms(50);
    char* ptr = NULL;
    do
    {
        ptr = fgets(receive_buff, READ_LINE_BUFSIZE, wifi_uart.rx);
        if(NULL != ptr)
        {
            puts(receive_buff);
        }
    } while(NULL != ptr);

    while(1)
    {
        run_station_module();
        //PCM_gotoLPM0();
        while(!rtc_second_passed)
        {
            MAP_PCM_gotoLPM0();
        }
        rtc_second_passed = false;
        MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
    }
}
