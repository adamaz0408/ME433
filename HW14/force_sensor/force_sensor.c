#include <stdio.h>
#include "pico/stdlib.h"


#define DT_PIN 2
#define SCK_PIN 3

#define MAX_SAMPLES 2000 // prevents overflowing Pico's RAM

// Global arrays to buffer data so we don't crash memory during collection
int32_t raw_data[MAX_SAMPLES];
float filtered_data[MAX_SAMPLES];
uint32_t time_data[MAX_SAMPLES];

void hx711_init() {
    gpio_init(DT_PIN);
    gpio_set_dir(DT_PIN, GPIO_IN);
    
    gpio_init(SCK_PIN);
    gpio_set_dir(SCK_PIN, GPIO_OUT);
    gpio_put(SCK_PIN, 0); // SCK must start low
}

// bit-banging driver
int32_t hx711_read() {
    // data is ready
    while (gpio_get(DT_PIN) == 1) {
        tight_loop_contents();
    }

    uint32_t raw = 0;

    // pulse clock 24 times to read 24 bits
    for (int i = 0; i < 24; i++) {
        gpio_put(SCK_PIN, 1);
        sleep_us(1);
        
        raw = (raw << 1) | gpio_get(DT_PIN); // shift and read
        
        gpio_put(SCK_PIN, 0);
        sleep_us(1);
    }

    // 25th pulse (sets gain to 128 for next reading)
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

int main() {
    // init standard I/O (USB Virtual COM Port)
    stdio_init_all();
    
    // init HX711 pins
    hx711_init();

    sleep_ms(2000); 

    // IIR Filter constant
    float alpha = 0.2f; 
    float current_filtered = 0.0f;
    bool first_reading = true;

    while (true) {
        int requested_samples = 0;
        
        // block and wait for command from python script
        scanf("%d", &requested_samples);

        if (requested_samples > 0 && requested_samples <= MAX_SAMPLES) {
            
            // data collection phase
            for (int i = 0; i < requested_samples; i++) {
                int32_t raw_val = hx711_read();
                
                // init filter on first reading to prevent massive jump
                if (first_reading) {
                    current_filtered = (float)raw_val;
                    first_reading = false;
                }

                // apply IIR filter
                current_filtered = (alpha * (float)raw_val) + ((1.0f - alpha) * current_filtered);

                // store data
                raw_data[i] = raw_val;
                filtered_data[i] = current_filtered;
                time_data[i] = to_ms_since_boot(get_absolute_time());
            }

            // data transmission phase
            for (int i = 0; i < requested_samples; i++) {
                // Print back in a clean CSV format: Time, Raw, Filtered
                printf("%lu, %ld, %.2f\r\n", time_data[i], raw_data[i], filtered_data[i]);
            }
            
            printf("DONE\r\n"); 
        }
    }
    return 0;
}