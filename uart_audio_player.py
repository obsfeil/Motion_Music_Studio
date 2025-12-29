#!/usr/bin/env python3
"""
UART Audio Player for MSPM0 Synthesizer
Receives audio samples via UART and plays them in real-time
"""

import serial
import sounddevice as sd
import numpy as np
import struct
import sys

# Configuration
SERIAL_PORT = 'COM5'  # Change to your port (Windows: COM3, Linux: /dev/ttyUSB0)
BAUD_RATE = 921600
SAMPLE_RATE = 1000  # We're receiving decimated samples (1 kHz)
BUFFER_SIZE = 100   # Number of samples to buffer

def find_serial_port():
    """Find available serial ports"""
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()
    
    print("Available serial ports:")
    for i, port in enumerate(ports):
        print(f"  {i}: {port.device} - {port.description}")
    
    if not ports:
        print("No serial ports found!")
        return None
    
    # Try to find MSPM0 board
    for port in ports:
        if "XDS" in port.description or "UART" in port.description:
            return port.device
    
    # Otherwise return first port
    return ports[0].device

def receive_samples(ser, num_samples):
    """Receive audio samples from UART"""
    samples = []
    
    for _ in range(num_samples):
        # Read 2 bytes (16-bit sample)
        data = ser.read(2)
        
        if len(data) != 2:
            break
        
        # Convert to signed 16-bit integer (little-endian)
        sample = struct.unpack('<h', data)[0]
        samples.append(sample)
    
    return np.array(samples, dtype=np.int16)

def upsample_audio_simple(samples, ratio=48):
    """Simple upsampling by repeating samples (no scipy needed)"""
    upsampled = np.repeat(samples, ratio)
    return upsampled

def main():
    # Find serial port
    port = find_serial_port()
    if not port:
        print("Please specify serial port manually")
        return
    
    print(f"\nOpening {port} at {BAUD_RATE} baud...")
    
    try:
        # Open serial port
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        
        print("Connected! Starting audio playback...")
        print("Press Ctrl+C to stop\n")
        
        # Create audio stream with upsampled rate
        stream = sd.OutputStream(
            samplerate=48000,  # Upsampled to 48 kHz
            channels=1,
            dtype='int16',
            blocksize=BUFFER_SIZE * 48  # Larger buffer for upsampled data
        )
        
        stream.start()
        
        # Main loop
        sample_count = 0
        while True:
            # Receive samples at 1 kHz
            samples = receive_samples(ser, BUFFER_SIZE)
            
            if len(samples) == 0:
                continue
            
            # Upsample to 48 kHz (repeat each sample 48 times)
            upsampled = upsample_audio_simple(samples, ratio=48)
            
            # Play audio
            stream.write(upsampled)
            
            # Print status
            sample_count += len(samples)
            sys.stdout.write(f"\rReceived: {sample_count} samples | "
                           f"Latency: {stream.latency*1000:.1f}ms    ")
            sys.stdout.flush()
    
    except KeyboardInterrupt:
        print("\n\nStopping...")
    
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()
    
    finally:
        if 'stream' in locals():
            stream.stop()
            stream.close()
        if 'ser' in locals():
            ser.close()
        print("Closed")

if __name__ == "__main__":
    main()