#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define MCP23008_ADDR 0x20
#define IODIR_REG     0x00 // Direction register: 1 = Input, 0 = Output
#define GPIO_REG      0x09 // Port register for reading inputs
#define OLAT_REG      0x0A // Output Latch register for writing outputs

#define I2C_SDA_PIN   4
#define I2C_SCL_PIN   5
#define HEARTBEAT_PIN 14 

void setPin(unsigned char address, unsigned char reg, unsigned char value) {
    unsigned char buf[2];
    buf[0] = reg;
    buf[1] = value;
    // write 2 bytes (register address + value), false means we release the bus when done
    i2c_write_blocking(i2c_default, address, buf, 2, false);
}

unsigned char readPin(unsigned char address, unsigned char reg) {
    unsigned char buf;
    // true means keep host control of bus (no stop bit)
    i2c_write_blocking(i2c_default, address, &reg, 1, true);
    // false means done with bus
    i2c_read_blocking(i2c_default, address, &buf, 1, false);
    return buf;
}

int main() {
    stdio_init_all();

    // initialize heartbeat LED
    gpio_init(HEARTBEAT_PIN);
    gpio_set_dir(HEARTBEAT_PIN, GPIO_OUT);

    // initialize I2C peripheral
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    
    // enable internal pullups as backup safety
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // initialize MCP23008 Direction
    // IODIR is 0xFF (all 1s / all inputs)
    // want GP7 as output (0) and GP0 as input (1)
    // binary 0111 1111 equals 0x7F in hex
    setPin(MCP23008_ADDR, IODIR_REG, 0x7F); 

    int loop_counter = 0;

    while (1) {
        // read current state of all input pins
        unsigned char current_gpio = readPin(MCP23008_ADDR, GPIO_REG);
        // read current state of all output latches
        unsigned char current_olat = readPin(MCP23008_ADDR, OLAT_REG);

        // bitwise logic check
        if ((current_gpio & 0x01) == 0) {
            // pushed button: GP7 ON
            // Bitwise OR (0x80) forces bit 7 to 1
            current_olat = current_olat | 0x80; 
        } else {
            // unpushed button: GP7 OFF
            // Bitwise AND with inverse of 0x80 (~0x80) forces bit 7 to 0
            current_olat = current_olat & ~0x80; 
        }

        // write newly calculated states back to the Output Latch
        setPin(MCP23008_ADDR, OLAT_REG, current_olat);

        // heartbeat logic
        // toggle heartbeat LED every 10 loops
        if (loop_counter % 10 == 0) {
            gpio_put(HEARTBEAT_PIN, !gpio_get(HEARTBEAT_PIN));
        }
        
        loop_counter++;
        sleep_ms(50);
    }
}