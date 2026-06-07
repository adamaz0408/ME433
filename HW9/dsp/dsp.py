import csv
import matplotlib.pyplot as plt
import numpy as np

# load data for plotting
t = []
signal = []

filename = 'dsp/csv_data/sigD.csv'

with open(filename, 'r') as f:
    reader = csv.reader(f)
    for row in reader:
        try:
            t.append(float(row[0]))
            signal.append(float(row[1]))
        except ValueError:
            pass # skip header row if it exists

# calc sample rate
num_points = len(t)
total_time = t[-1] - t[0]
sample_rate = num_points / total_time

print(f"File: {filename}")
print(f"Total Data Points: {num_points}")
print(f"Total Time: {total_time} seconds")
print(f"Calculated Sample Rate (Fs): {sample_rate} Hz")

# FFT calc
# create freq x-axis 
k = np.arange(num_points)
T = num_points / sample_rate
frq = k / T
frq = frq[range(int(num_points / 2))] 

# calc FFT mags (y-axis)
Y = np.fft.fft(signal) / num_points
Y = Y[range(int(num_points / 2))]

# moving average filter (MAF) implementation
X = 15 # window size (guess aand check value for each CSV)
maf_signal = np.zeros(num_points)

# MAF loop
for i in range(X - 1, num_points):
    # slice last X elements and average them
    maf_signal[i] = np.mean(signal[i - X + 1 : i + 1])

# calc FFT for new filtered signal
Y_maf = np.fft.fft(maf_signal) / num_points
Y_maf = Y_maf[range(int(num_points / 2))]

# plotting both subplots
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

# signal v time plot
ax1.plot(t, signal, 'k', label='Unfiltered Signal')
ax1.plot(t, maf_signal, 'r', label='Filtered Signal')
ax1.set_xlabel('Time [s]')
ax1.set_ylabel('Amplitude')
ax1.set_title(f'MAF Filter ({filename}) - Window: {X}')
ax1.legend()
ax1.grid(True)

# FFT plot
ax2.plot(frq, abs(Y), 'k', label='Unfiltered FFT') 
ax2.plot(frq, abs(Y_maf), 'r', label='Filtered FFT')
ax2.set_xlabel('Freq (Hz)')
ax2.set_ylabel('|Y(freq)|')
ax2.set_title('FFT Comparison')
ax2.set_xlim(0, 500) # limit for visibility
ax2.legend()
ax2.grid(True)

plt.tight_layout()
plt.show()