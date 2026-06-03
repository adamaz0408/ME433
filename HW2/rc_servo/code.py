import time
import board
import pwmio
import digitalio

# 1. Power Indicator Configuration
led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT
led.value = True

# 2. PWM Configuration (GP15, 50Hz Period)
servo_pwm = pwmio.PWMOut(board.GP15, frequency=50, duty_cycle=0)

def set_servo_angle(pwm_out, angle):
    # Updated 16-bit limits for the 9g micro servo (1.5% to 12%)
    min_duty = 983   # 1.5% Duty Cycle
    max_duty = 7864  # 12% Duty Cycle
    
    if angle < 0:
        angle = 0
    if angle > 180:
        angle = 180
        
    duty = int(min_duty + (angle / 180.0) * (max_duty - min_duty))
    pwm_out.duty_cycle = duty

# 3. Sweep Loop Operation
current_angle = 0
angle_increment = 2

while True:
    set_servo_angle(servo_pwm, current_angle)
    
    current_angle += angle_increment
    
    if current_angle >= 180 or current_angle <= 0:
        angle_increment = -angle_increment
        
    time.sleep(0.03)
