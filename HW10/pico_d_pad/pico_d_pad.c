#include <stdio.h>
#include "pico/stdlib.h"

// Define the GPIO pins for the D-Pad
#define BTN_UP 2
#define BTN_DOWN 3
#define BTN_LEFT 4
#define BTN_RIGHT 5

int main() {
    // Initialize standard I/O (required for printf to work over USB)
    stdio_init_all();

    // Initialize the GPIO pins
    gpio_init(BTN_UP);
    gpio_init(BTN_DOWN);
    gpio_init(BTN_LEFT);
    gpio_init(BTN_RIGHT);

    // Set pins as inputs
    gpio_set_dir(BTN_UP, GPIO_IN);
    gpio_set_dir(BTN_DOWN, GPIO_IN);
    gpio_set_dir(BTN_LEFT, GPIO_IN);
    gpio_set_dir(BTN_RIGHT, GPIO_IN);

    // Enable internal pull-up resistors
    gpio_pull_up(BTN_UP);
    gpio_pull_up(BTN_DOWN);
    gpio_pull_up(BTN_LEFT);
    gpio_pull_up(BTN_RIGHT);

    // Give USB a moment to initialize before blasting data
    sleep_ms(2000); 

    while (true) {
        // Read pins. Because they are tied to ground (active-low), 
        // a pressed button reads 0. We use the NOT operator (!) to invert this 
        // so that 1 = Pressed and 0 = Unpressed.
        int up = !gpio_get(BTN_UP);
        int down = !gpio_get(BTN_DOWN);
        int left = !gpio_get(BTN_LEFT);
        int right = !gpio_get(BTN_RIGHT);

        // Print comma-separated string with a newline
        printf("%d,%d,%d,%d\n", up, down, left, right);

        // Sleep for ~33ms to achieve a ~30Hz update rate
        sleep_ms(33);
    }
    return 0;
}