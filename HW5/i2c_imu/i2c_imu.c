#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

// PICO PIN DEFINITIONS
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define HEARTBEAT_PIN 14

// MPU6050 REGISTERS
#define MPU6050_ADDR 0x68
#define ACCEL_CONFIG 0x1C
#define GYRO_CONFIG  0x1B
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B
#define WHO_AM_I     0x75

// I2C Helpers
void mpu6050_read_reg(uint8_t reg, uint8_t *buf, uint16_t len) {
    i2c_write_blocking(i2c_default, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c_default, MPU6050_ADDR, buf, len, false);
}

void mpu6050_write_reg(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    i2c_write_blocking(i2c_default, MPU6050_ADDR, buf, 2, false);
}

void mpu6050_setup() {
    mpu6050_write_reg(PWR_MGMT_1, 0x00); // wake up
    mpu6050_write_reg(ACCEL_CONFIG, 0x00); // +/- 2g
    mpu6050_write_reg(GYRO_CONFIG, 0x18); // +/- 2000 dps
}

// BRESENHAM'S LINE ALGORITHM
void drawLine(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        // prevent drawing off screen and crashing buffer
        if (x0 >= 0 && x0 < 128 && y0 >= 0 && y0 < 32) {
            ssd1306_drawPixel(x0, y0, 1);
        }
        
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

int main() {
    stdio_init_all();

    // init hardware
    gpio_init(HEARTBEAT_PIN);
    gpio_set_dir(HEARTBEAT_PIN, GPIO_OUT);

    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);

    sleep_ms(250);

    // trap
    uint8_t chip_id;
    mpu6050_read_reg(WHO_AM_I, &chip_id, 1);
    if (chip_id != 0x68 && chip_id != 0x98) {
        while (1) {
            gpio_put(HEARTBEAT_PIN, 1); sleep_ms(50);
            gpio_put(HEARTBEAT_PIN, 0); sleep_ms(50);
        }
    }

    mpu6050_setup();
    ssd1306_setup();

    uint8_t buffer[14];
    bool toggle_state = false;
    uint32_t last_blink_time = to_us_since_boot(get_absolute_time());

    while (1) {
        // read IMU data
        mpu6050_read_reg(ACCEL_XOUT_H, buffer, 14);

        int16_t accel_x = (buffer[0] << 8) | buffer[1];
        int16_t accel_y = (buffer[2] << 8) | buffer[3];
        
        float ax_g = accel_x * 0.000061f;
        float ay_g = accel_y * 0.000061f;

        // clear screen buffer for next frame
        ssd1306_clear();

        // map vectors to pixels
        int center_x = 64;
        int center_y = 16;
        
        // multiplier limits line length to 15 pixels so it stays on the screen
        int end_x = center_x + (int)(ax_g * -15.0f);
        int end_y = center_y + (int)(ay_g * 15.0f);

        // draw the Line
        drawLine(center_x, center_y, end_x, end_y);

        // push the frame to the OLED
        ssd1306_update();

        // independent heartbeat timer
        if (to_us_since_boot(get_absolute_time()) - last_blink_time > 500000) {
            toggle_state = !toggle_state;
            gpio_put(HEARTBEAT_PIN, toggle_state);
            last_blink_time = to_us_since_boot(get_absolute_time()); 
        }
    }
}