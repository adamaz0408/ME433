#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// pico pin definitions
#define PIN_MISO 16 
#define PIN_DAC_CS 17
#define PIN_SCK 18 
#define PIN_MOSI 19 
#define PIN_RAM_CS 20

// RAM command definitions
#define RAM_WRSR 0x01
#define RAM_WRITE 0x02
#define RAM_READ  0x03
#define RAM_SEQ   0x40 

static inline void ram_cs_select() {
    asm volatile("nop \n nop \n nop"); 
    gpio_put(PIN_RAM_CS, 0);
    asm volatile("nop \n nop \n nop"); 
}

static inline void ram_cs_deselect() {
    asm volatile("nop \n nop \n nop"); 
    gpio_put(PIN_RAM_CS, 1);
    asm volatile("nop \n nop \n nop"); 
}

static inline void dac_cs_select() {
    asm volatile("nop \n nop \n nop"); 
    gpio_put(PIN_DAC_CS, 0);
    asm volatile("nop \n nop \n nop"); 
}

static inline void dac_cs_deselect() {
    asm volatile("nop \n nop \n nop"); 
    gpio_put(PIN_DAC_CS, 1);
    asm volatile("nop \n nop \n nop"); 
}

// RAM init
void spi_ram_init() {
    // array containing Write Command and Mode Data
    uint8_t setup_cmd[2] = {RAM_WRSR, RAM_SEQ};

    // send command
    ram_cs_select();
    spi_write_blocking(spi_default, setup_cmd, 2);
    ram_cs_deselect();
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

    dac_cs_deselect();
    spi_write_blocking(spi_default, data, 2);
    dac_cs_deselect();
}

int main() {
    stdio_init_all();

    // SPI init
    spi_init(spi_default, 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // CS init (both chips)
    gpio_init(PIN_DAC_CS);
    gpio_set_dir(PIN_DAC_CS, GPIO_OUT);
    dac_cs_deselect();

    gpio_init(PIN_RAM_CS);
    gpio_set_dir(PIN_RAM_CS, GPIO_OUT);
    ram_cs_deselect();

    sleep_ms(250);
    spi_ram_init();

    // pre-comp engine
    uint8_t wave_buffer[2000];

    for(int i = 0; i < 1000; i++) {
        // calc sine wave
        float sine_val = 511.5f * sin(2.0f * M_PI * (i / 1000.0f)) + 511.5f;
        
        // bitwise formatting for MCP4912 DAC
        uint16_t command = (0 << 15) | 0x3000 | ((uint16_t)sine_val << 2);
        
        // store high byte and low byte in array
        wave_buffer[i * 2]     = (command >> 8) & 0xFF; 
        wave_buffer[i * 2 + 1] = command & 0xFF;        
    }

    // write all 2000 bytes into RAM starting at 0x0000
    uint8_t write_header[3] = {RAM_WRITE, 0x00, 0x00};

    ram_cs_select();
    spi_write_blocking(spi_default, write_header, 3); // send instruction and address
    spi_write_blocking(spi_default, wave_buffer, 2000); // blast array into memory
    ram_cs_deselect();
    
    // playback loop
    uint16_t current_address = 0;

    while (1) {
        // read from RAM
        uint8_t read_header[3] = {RAM_READ, (current_address >> 8) & 0xFF, current_address & 0xFF};
        uint8_t dac_data[2];

        ram_cs_select();
        spi_write_blocking(spi_default, read_header, 3); // tell RAM where to look
        spi_read_blocking(spi_default, 0, dac_data, 2);  // grab exactly 2 bytes
        ram_cs_deselect();

        // write to DAC
        dac_cs_select();
        spi_write_blocking(spi_default, dac_data, 2);
        dac_cs_deselect();

        // advance memory pointer
        current_address += 2;
        
        if (current_address >= 2000) {
            current_address = 0; 
        }

        // 1ms delay * 1000 sampers (1Hz)
        sleep_ms(1);
    }
}