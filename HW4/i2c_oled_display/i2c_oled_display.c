#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "font.h"

#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define HEARTBEAT_PIN 14 

// draw single char from font.h matrix
void drawChar(int x, int y, char c) {
    // only process printable ASCII chars
    if (c < ' ' || c > '~') {
        return; 
    }

    // offset ASCII value by 32 to find array index
    int font_index = c - ' ';

    // loop through 5 vertical columns of char
    for (int col = 0; col < 5; col++) {
        // grab byte representing specific col
        char column_data = ASCII[font_index][col];

        // loop through 8 vertical pixels in col
        for (int row = 0; row < 8; row++) {
            // check if bit at row pos is 1
            bool pixel_state = (column_data & (1 << row)) != 0;
            
            // draw pixel offset from x and y coord
            ssd1306_drawPixel(x + col, y + row, pixel_state);
        }
    }
}

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

    // draw "ME" onscreen
    drawChar(10, 10, 'M');
    drawChar(16, 10, 'E');
    ssd1306_update(); // push buffer to clear any static on screen

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