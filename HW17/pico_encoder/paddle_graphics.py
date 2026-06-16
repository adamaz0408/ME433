import serial
import pygame
import sys

COM_PORT = 'COM3' 
BAUD_RATE = 115200

# graphics config
WINDOW_WIDTH = 800
WINDOW_HEIGHT = 600
PADDLE_WIDTH = 200
PADDLE_HEIGHT = 40
CENTER_X = WINDOW_WIDTH // 2
CENTER_Y = WINDOW_HEIGHT // 2

def map_value(value, in_min, in_max, out_min, out_max):
    """A standard mechatronics mapping function (like Arduino's map())"""
    # constrain value to input range to avoid extreme color/angle glitches
    value = max(min(value, in_max), in_min)
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

def main():
    # init pygame
    pygame.init()
    screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
    pygame.display.set_caption("Adam's Haptic Paddle Visualizer")
    clock = pygame.time.Clock()
    font = pygame.font.SysFont(None, 36)

    # establish serial connection to Pico
    print(f"Connecting to Pico on {COM_PORT}...")
    try:
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.1)
    except Exception as e:
        print(f"Failed to connect to Serial: {e}")
        return

    # create base surface for paddle
    paddle_surface = pygame.Surface((PADDLE_WIDTH, PADDLE_HEIGHT), pygame.SRCALPHA)
    
    # state variables
    current_angle = 0.0
    current_force = 0.0

    print("Graphics Engine Running. Press ESC or close the window to exit.")

    running = True
    while running:
        # event handling
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False

        # serial parsing
        while ser.in_waiting:
            try:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    parts = line.split(',')
                    if len(parts) == 2:
                        raw_angle = int(parts[0])
                        raw_force = float(parts[1])
                        
                        # convert 12-bit raw angle (0-4095) to Degrees (0-360)
                        current_angle = map_value(raw_angle, 0, 4095, 0, 360)
                        current_force = raw_force
            except (ValueError, UnicodeDecodeError):
                pass 

        # graphics calculations
        red_val = map_value(abs(current_force), 0, 100000, 0, 255)
        green_val = 255 - red_val
        paddle_color = (int(red_val), int(green_val), 0)

        # fill base surface with calc color
        paddle_surface.fill(paddle_color)

        # rotate paddle surface based on AS5600 angle
        rotated_paddle = pygame.transform.rotate(paddle_surface, current_angle)
        
        # get new rect bounding box so it rotates cleanly around its true center
        rect = rotated_paddle.get_rect(center=(CENTER_X, CENTER_Y))

        screen.fill((30, 30, 30)) 

        # draw pivot point (motor shaft)
        pygame.draw.circle(screen, (200, 200, 200), (CENTER_X, CENTER_Y), 10)

        # draw rotated paddle
        screen.blit(rotated_paddle, rect.topleft)

        # draw telemetry text overlay
        text_angle = font.render(f"Angle: {current_angle:.1f} deg", True, (255, 255, 255))
        text_force = font.render(f"Force: {current_force:.0f} raw", True, (255, 255, 255))
        screen.blit(text_angle, (20, 20))
        screen.blit(text_force, (20, 60))

        # update screen and lock framerate
        pygame.display.flip()
        clock.tick(60)

    # cleanup
    ser.close()
    pygame.quit()
    sys.exit()

if __name__ == '__main__':
    main()