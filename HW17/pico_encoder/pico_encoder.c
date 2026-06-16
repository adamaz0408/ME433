#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// HX711 (Load Cell)
#define DT_PIN 2
#define SCK_PIN 3

// AS5600 (Magnetic Encoder)
#define I2C_PORT i2c0
#define SDA_PIN 4
#define SCL_PIN 5

#define AS5600_ADDR         0x36
#define AS5600_RAW_ANGLE_HI 0x0C


// HX711 hardware drivers
void hx711_init() {
    gpio_init(DT_PIN);
    gpio_set_dir(DT_PIN, GPIO_IN);
    
    gpio_init(SCK_PIN);
    gpio_set_dir(SCK_PIN, GPIO_OUT);
    gpio_put(SCK_PIN, 0); 
}

int32_t hx711_read() {
    // block until data is ready
    while (gpio_get(DT_PIN) == 1) {
        tight_loop_contents();
    }

    uint32_t raw = 0;

    for (int i = 0; i < 24; i++) {
        gpio_put(SCK_PIN, 1);
        sleep_us(1); 
        raw = (raw << 1) | gpio_get(DT_PIN); 
        gpio_put(SCK_PIN, 0);
        sleep_us(1);
    }

    // 25th pulse for 128x gain
    gpio_put(SCK_PIN, 1);
    sleep_us(1);
    gpio_put(SCK_PIN, 0);
    sleep_us(1);

    // sign-extend 24-bit two's complement to 32-bit signed int
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }

    return (int32_t)raw;
}


// AS5600 hardware drivers
void as5600_init() {
    // init I2C at 400 kHz
    i2c_init(I2C_PORT, 400 * 1000);
    
    // assign GPIO functions to I2C
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    
    // I2C requires lines to be pulled high
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
}

uint16_t as5600_read_angle() {
    uint8_t reg = AS5600_RAW_ANGLE_HI;
    uint8_t buffer[2] = {0, 0};

    // point AS5600 to High Byte register
    i2c_write_blocking(I2C_PORT, AS5600_ADDR, &reg, 1, true);

    // read exactly 2 bytes back
    i2c_read_blocking(I2C_PORT, AS5600_ADDR, buffer, 2, false);

    // bitwise Math: shift High Byte left by 8, then OR it with Low Byte
    uint16_t raw_angle = (buffer[0] << 8) | buffer[1];
    
    return raw_angle;
}


int main() {
    stdio_init_all();
    
    // init both sensor modules
    hx711_init();
    as5600_init();

    sleep_ms(2000); 

    // IIR filter config for load cell
    float alpha = 0.2f; 
    float filtered_force = 0.0f;
    bool first_reading = true;

    while (true) {
        // read analog front end
        int32_t raw_force = hx711_read();
        
        if (first_reading) {
            filtered_force = (float)raw_force;
            first_reading = false;
        }

        filtered_force = (alpha * (float)raw_force) + ((1.0f - alpha) * filtered_force);

        // read magnetic encoder
        uint16_t angle = as5600_read_angle();

        // telemetry stream
        printf("%u, %.2f\r\n", angle, filtered_force);
    }
    
    return 0;
}