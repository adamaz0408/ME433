import serial
import time
import matplotlib.pyplot as plt

COM_PORT = 'COM3'
BAUD_RATE = 115200
SAMPLES = 400

def main():
    indices = []
    desired_currents = []
    actual_currents = []

    print(f"Connecting to STM32 on {COM_PORT}...")
    try:
        with serial.Serial(COM_PORT, BAUD_RATE, timeout=2) as ser:
            time.sleep(2) # give port a second to open
            
            print("Sending trigger command 'a'...")
            ser.write(b'a')
            
            print("Waiting for step response data...")
            
            # read number of samples we expect
            lines_read = 0
            while lines_read < SAMPLES:
                line = ser.readline().decode('utf-8').strip()
                
                if not line:
                    continue
                    
                # parse incoming CSV format: Index, Desired, Actual
                try:
                    parts = line.split(',')
                    if len(parts) == 3:
                        indices.append(int(parts[0]))
                        desired_currents.append(int(parts[1]))
                        actual_currents.append(int(parts[2]))
                        lines_read += 1
                except ValueError:
                    pass
                    
    except Exception as e:
        print(f"Serial Error: {e}")
        print("Make sure STM32CubeIDE's Serial Monitor is CLOSED before running this!")
        return

    print("Data received! Plotting...")

    # plotting
    plt.figure(figsize=(10, 6))
    
    plt.plot(indices, desired_currents, label='Desired Current (mA)', color='black', linestyle='--')
    plt.plot(indices, actual_currents, label='Actual Current (mA)', color='red')
    
    plt.title('Motor Current PI Controller Step Response')
    plt.xlabel('Time (milliseconds)')
    plt.ylabel('Current (mA)')
    plt.legend()
    plt.grid(True)
    
    plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    main()