import pyaudio
import pyaudio
import wave
import numpy as np
import pandas as pd
from scipy.fft import rfft, rfftfreq

# Audio parameters
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 44100  # Sample rate
CHUNK = 1024  # Buffer size
RECORD_SECONDS = 5
OUTPUT_CSV = "frequencies.csv"

# Initialize PyAudio
audio = pyaudio.PyAudio()

# Start recording
stream = audio.open(format=FORMAT, channels=CHANNELS,
                    rate=RATE, input=True,
                    frames_per_buffer=CHUNK)

print("Recording...")
frames = []
for _ in range(0, int(RATE / CHUNK * RECORD_SECONDS)):
    data = stream.read(CHUNK)
    frames.append(np.frombuffer(data, dtype=np.int16))  # Convert to NumPy array

print("Recording finished.")

# Stop and close the stream
stream.stop_stream()
stream.close()
audio.terminate()

# Convert recorded frames to a NumPy array
audio_signal = np.concatenate(frames, axis=0)

# Perform FFT (Fast Fourier Transform)
N = len(audio_signal)  # Number of samples
yf = np.abs(rfft(audio_signal))  #
xf = rfftfreq(N, 1 / RATE)  # Frequency bins
# Save to CSV
df = pd.DataFrame({"Frequency (Hz)": xf, "Magnitude": yf})
df.to_csv(OUTPUT_CSV, index=False)

print(f"Frequency data saved to {OUTPUT_CSV}")