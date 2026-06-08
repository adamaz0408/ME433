import csv
import matplotlib.pyplot as plt
import numpy as np
from scipy.signal import convolve

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

# infinite impulse response (IIR) filter implementation
A = 0.80 # weight for past avg (guess aand check value for each CSV)
B = 1.0 - A

iir_signal = np.zeros(num_points)
iir_signal[0] = signal[0] # seed first value

# IIR Loop
for i in range(1, num_points):
    iir_signal[i] = A * iir_signal[i-1] + B * signal[i]

# calc FFT for new filtered signal
Y_iir = np.fft.fft(iir_signal) / num_points
Y_iir = Y_iir[range(int(num_points / 2))]

# FIR filter implementation
taps = 51 # same for all CSV
window = "Hamming" # same for all CSV
fc = 25 # adjusted for each CSV
bandwidth = 15 # adjusted for each CSV
fir_weights = np.array([ # slightly adjusted for each CSV
    -0.00032, -0.00063, -0.00095, -0.00125, -0.00150, -0.00166, -0.00169, 
    -0.00153, -0.00115, -0.00049, 0.00049, 0.00178, 0.00336, 0.00518, 
    0.00717, 0.00924, 0.01129, 0.01319, 0.01484, 0.01614, 0.01700, 0.01736, 
    0.01719, 0.01648, 0.01526, 0.01358, 0.01526, 0.01648, 0.01719, 0.01736, 
    0.01700, 0.01614, 0.01484, 0.01319, 0.01129, 0.00924, 0.00717, 0.00518, 
    0.00336, 0.00178, 0.00049, -0.00049, -0.00115, -0.00153, -0.00169, 
    -0.00166, -0.00150, -0.00125, -0.00095, -0.00063, -0.00032
])
fir_signal = convolve(signal, fir_weights, mode='same') * 2.30

Y_fir = np.fft.fft(fir_signal) / num_points
Y_fir = Y_fir[:num_points // 2]

# plotting both subplots
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
title_str = (f"FIR Filter ({taps} Taps, {window}, Fc={fc}Hz, BW={bandwidth}Hz)\n"
             f"File: {filename}")
fig.suptitle(title_str)

# signal v time plot
ax1.plot(t, signal, 'k', label='Unfiltered Signal')
ax1.plot(t, fir_signal, 'r', label='Filtered Signal')
ax1.set_xlabel('Time [s]')
ax1.set_ylabel('Amplitude')
ax1.legend()
ax1.grid(True)

# FFT plot
ax2.plot(frq, abs(Y), 'k', label='Unfiltered FFT') 
ax2.plot(frq, abs(Y_fir), 'r', label='Filtered FFT')
ax2.set_xlabel('Freq (Hz)')
ax2.set_ylabel('|Y(freq)|')
ax2.set_title('FFT Comparison')
ax2.set_xlim(0, max(frq)/4) # limit for visibility
ax2.legend()
ax2.grid(True)

plt.tight_layout()
plt.show()