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

// print full string of chars
void drawMessage(int x, int y, char *message) {
    int i = 0;
    
    // loop through char array until reached null terminator
    while (message[i] != '\0') {
        drawChar(x, y, message[i]);
        
        // move cursor to right by 6 pixels
        x += 6;
        
        // screen wraparound logic
        if (x > (128 - 5)) {
            x = 0;
            y += 8;
        }
        
        i++;
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

    // loop variables
    bool toggle_state = false;
    uint32_t start_time, end_time;
    float fps = 0.0f; // initialize to 0 for first frame
    uint32_t last_blink_time = to_us_since_boot(get_absolute_time());

    while (1) {
        // record start time (ms)
        start_time = to_us_since_boot(get_absolute_time());

        ssd1306_clear(); // ensure buffer starts completely empty

        // read ADC and calc real voltage
        uint16_t adc_raw = adc_read();
        float voltage = (adc_raw * 3.3f) / 4095.0f;

        // draw "HELLO WORLD!" onscreen
        char hello_msg[50];
        sprintf(hello_msg, "HELLO WORLD!");
        drawMessage(28, 0, hello_msg);

        // draw live voltage in middle
        char adc_msg[50];
        sprintf(adc_msg, "ADC0: %.2f V", voltage);
        drawMessage(0, 12, adc_msg); 

        // draw FPS at bottom
        char fps_msg[50];
        sprintf(fps_msg, "FPS: %.1f", fps);
        drawMessage(0, 24, fps_msg); 

        // push completely built frame to display
        ssd1306_update();
        
        // calc how long frame took to gen FPS for next loop
        end_time = to_us_since_boot(get_absolute_time());
        fps = 1000000.0f / (end_time - start_time);

        // heartbeat logic
        if (to_us_since_boot(get_absolute_time()) - last_blink_time > 500000) {
            toggle_state = !toggle_state;
            gpio_put(HEARTBEAT_PIN, toggle_state);
            last_blink_time = to_us_since_boot(get_absolute_time()); // Reset the timer
            }
    }
}