#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define SERVO_PIN 15
#define LED_PIN 25

// Function to map 0-180 degrees to 1.5% - 12% duty cycle
void set_servo_angle(uint pin, float angle) {
    float min_pulse_us = 300.0f;  // 1.5% duty cycle (0.3 ms)
    float max_pulse_us = 2400.0f; // 12% duty cycle (2.4 ms)
    
    // clamp angle to prevent over-rotation
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;
    
    // linearly interpolate angle to pulse width
    float pulse_width = min_pulse_us + (angle / 180.0f) * (max_pulse_us - min_pulse_us);
    
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);
    
    pwm_set_chan_level(slice_num, channel, (uint16_t)pulse_width);
}

int main() {
    stdio_init_all();

    // initialize LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1); 

    // configure hardware PWM
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);

    pwm_config config = pwm_get_default_config();
    
    pwm_config_set_clkdiv(&config, 150.0f);
    
    pwm_config_set_wrap(&config, 20000);
    
    pwm_init(slice_num, &config, true);

    // sweep execution loop
    float current_angle = 0.0f;
    float sweep_step = 2.0f; 
    bool ascending = true;

    while (true) {
        set_servo_angle(SERVO_PIN, current_angle);
        
        if (ascending) {
            current_angle += sweep_step;
            if (current_angle >= 180.0f) {
                current_angle = 180.0f;
                ascending = false;
            }
        } else {
            current_angle -= sweep_step;
            if (current_angle <= 0.0f) {
                current_angle = 0.0f;
                ascending = true;
            }
        }
        
        sleep_ms(30); // controls how fast arm sweeps
    }
}