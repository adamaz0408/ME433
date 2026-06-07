import csv
import matplotlib.pyplot as plt

# load data for plotting
t = []
signal = []

filename = 'dsp/csv_data/sigA.csv'

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

# plot raw data to prove it worked
plt.plot(t, signal, 'k')
plt.xlabel('Time [s]')
plt.ylabel('Amplitude')
plt.title(f'Raw Signal vs Time ({filename})')
plt.grid(True)
plt.show()