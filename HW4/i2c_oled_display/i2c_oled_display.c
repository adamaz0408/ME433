#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306.h"

#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define HEARTBEAT_PIN 14 

int main() {
    stdio_init_all();

    // initialize heartbeat LED
    gpio_init(HEARTBEAT_PIN);
    gpio_set_dir(HEARTBEAT_PIN, GPIO_OUT);

    // initialize I2C for OLED
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    
    // enable internal pull-ups as fallback
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // initialize ADC
    adc_init();
    adc_gpio_init(26);

    // initialize SSD1306 OLED
    ssd1306_setup();
    ssd1306_clear(); // ensure buffer starts completely empty
    ssd1306_update(); // push empty buffer to clear any static on screen

    bool toggle_state = false;

    // 1Hz Execution Loop
    while (1) {
        // toggle state variable
        toggle_state = !toggle_state;

        // apply state to heartbeat LED
        gpio_put(HEARTBEAT_PIN, toggle_state);

        // apply state to single pixel in center of screen
        ssd1306_drawPixel(64, 16, toggle_state);
        
        // push updated buffer over I2C to display
        ssd1306_update();

        sleep_ms(500);
    }
}