import serial
import time
import numpy as np
import matplotlib.pyplot as plt

PORT = 'COM3'
BAUD = 115200
SAMPLES = 500

def main():
    times = []
    raw_data = []
    filtered_data = []

    print(f"Opening {PORT}...")
    try:
        with serial.Serial(PORT, BAUD, timeout=1) as ser:
            time.sleep(2) # give serial connection moment to init
            
            print(f"Requesting {SAMPLES} samples from the Pico...")
            ser.write(f"{SAMPLES}\n".encode('utf-8'))
            
            print("Receiving data... (Try pressing on the sensor now!)")
            while True:
                line = ser.readline().decode('utf-8').strip()
                
                if not line:
                    continue
                
                # check for termination string programmed in C
                if "DONE" in line:
                    print("Data collection complete!")
                    break
                
                # parse incoming CSV format: Time, Raw, Filtered
                try:
                    t, r, f = line.split(',')
                    times.append(int(t))
                    raw_data.append(int(r))
                    filtered_data.append(float(f))
                except ValueError:
                    pass 
                    
    except Exception as e:
        print(f"Serial Connection Error: {e}")
        print("Check your COM port and make sure no other Serial Monitors are open.")
        return

    # data prep
    if len(times) < 2:
        print("Not enough data collected to generate plots.")
        return

    # convert python lists to numpy arrays for mathematical operations
    t_arr = np.array(times)
    raw_arr = np.array(raw_data)
    filt_arr = np.array(filtered_data)

    t_sec = (t_arr - t_arr[0]) / 1000.0 

    # calc actual sample rate from timestamps
    dt = np.mean(np.diff(t_sec))
    fs = 1.0 / dt
    print(f"Calculated Sample Rate: {fs:.2f} Hz")

    # FFT calc
    n = len(t_sec)
    freqs = np.fft.fftfreq(n, d=dt)[:n//2] 
    
    # remove DC offset
    raw_ac = raw_arr - np.mean(raw_arr)
    filt_ac = filt_arr - np.mean(filt_arr)
    
    # compute magnitude of FFT
    fft_raw = np.abs(np.fft.fft(raw_ac))[:n//2] * 2.0 / n
    fft_filt = np.abs(np.fft.fft(filt_ac))[:n//2] * 2.0 / n

    # plotting
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

    # time domian
    ax1.plot(t_sec, raw_arr, label='Raw Data', alpha=0.5, color='blue')
    ax1.plot(t_sec, filt_arr, label='IIR Filtered', linewidth=2, color='red')
    ax1.set_title('Load Cell Force vs. Time')
    ax1.set_xlabel('Time (Seconds)')
    ax1.set_ylabel('ADC Value')
    ax1.legend()
    ax1.grid(True)

    # FFT
    ax2.plot(freqs, fft_raw, label='Raw FFT', alpha=0.5, color='blue')
    ax2.plot(freqs, fft_filt, label='Filtered FFT', linewidth=2, color='red')
    ax2.set_title('Frequency Analysis (Looking for 25-30Hz Noise)')
    ax2.set_xlabel('Frequency (Hz)')
    ax2.set_ylabel('Magnitude')
    ax2.set_xlim(0, 40)
    ax2.legend()
    ax2.grid(True)

    plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    main()