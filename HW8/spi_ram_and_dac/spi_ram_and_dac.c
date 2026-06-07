#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define PIN_CS   17 
#define PIN_SCK  18 
#define PIN_MOSI 19 

static inline void cs_select() {
    asm volatile("nop \n nop \n nop"); 
    gpio_put(PIN_CS, 0);
    asm volatile("nop \n nop \n nop"); 
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop"); 
    gpio_put(PIN_CS, 1);
    asm volatile("nop \n nop \n nop"); 
}

// bitwise engine
void setVoltage(uint8_t channel, uint16_t voltage) {
    if (voltage > 1023) voltage = 1023;

    // bitwise assembly of 16-bit command
    uint16_t command = (channel << 15) | 0x3000 | (voltage << 2);

    // split into high byte and low byte for SPI transfer
    uint8_t data[2];
    data[0] = (command >> 8) & 0xFF;
    data[1] = command & 0xFF;

    cs_select();
    spi_write_blocking(spi_default, data, 2);
    cs_deselect();
}

int main() {
    stdio_init_all();

    // SPI init
    spi_init(spi_default, 12 * 1000); 
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // CS init
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    sleep_ms(250); // Let DAC boot

    // signal generation loop
    int t_ms = 0;

    while (1) {
        // CH1: 2Hz Sine Wave
        float sine_val = 511.5f * sin(2.0f * M_PI * 2.0f * (t_ms / 1000.0f)) + 511.5f;
        setVoltage(0, (uint16_t)sine_val);

        // CH2: 1Hz Triangle Wave
        int phase = t_ms % 1000;
        uint16_t tri_val;
        
        if (phase < 500) {
            tri_val = (phase * 1023) / 500; 
        } else {
            tri_val = 1023 - (((phase - 500) * 1023) / 500); 
        }
        setVoltage(1, tri_val);

        // 200Hz loop execution rate
        sleep_ms(5);
        t_ms += 5;

        // reset tracker exactly every 1000ms to prevent integer overflow
        if (t_ms >= 1000) {
            t_ms -= 1000;
        }
    }
}