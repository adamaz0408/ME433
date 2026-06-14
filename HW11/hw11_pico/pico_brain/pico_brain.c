#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0 
#define UART_RX_PIN 1 

int main() {
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);
    
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    sleep_ms(2000); 

    while (true) {
        // Listen to the Computer (USB)
        int usb_char = getchar_timeout_us(0);
        if (usb_char != PICO_ERROR_TIMEOUT) {
            uart_putc(UART_ID, (char)usb_char);
        }

        // Listen to the STM32 (Physical Wire)
        if (uart_is_readable(UART_ID)) {
            char hw_char = uart_getc(UART_ID);
            putchar(hw_char);
            stdio_flush(); 
        }
    }
    return 0;
}