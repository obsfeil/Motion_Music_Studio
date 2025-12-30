#!/usr/bin/env python3
"""
UART Audio Player for MSPM0 Synthesizer - OPTIMIZED
Receives audio samples via UART and plays them in real-time

IMPROVEMENTS:
- 4 kHz sample rate (better quality)
- Optimized buffer size
- Better error handling
"""

import serial
import sounddevice as sd
import numpy as np
import struct
import sys

# Configuration - OPTIMIZED FOR 4 kHz
BAUD_RATE = 921600
SAMPLE_RATE = 4000      # ✅ Changed from 1000 to 4000
BUFFER_SIZE = 200       # ✅ Optimized buffer
UPSAMPLE_RATIO = 12     # ✅ 48000 / 4000 = 12

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
    
    # Try to find Application/User UART
    for port in ports:
        if "Application/User UART" in port.description:
            print(f"✅ Auto-selected: {port.device}")
            return port.device
    
    # Otherwise return first port
    return ports[0].device

def receive_samples(ser, num_samples):
    """Receive audio samples from UART with error handling"""
    samples = []
    
    for _ in range(num_samples):
        try:
            # Read 2 bytes (16-bit sample, little-endian)
            data = ser.read(2)
            
            if len(data) != 2:
                # Not enough data, pad with silence
                samples.append(0)
                continue
            
            # Convert to signed 16-bit integer
            sample = struct.unpack('<h', data)[0]
            
            # Sanity check (reject obvious noise)
            if abs(sample) > 32000:
                samples.append(0)  # Replace with silence
            else:
                samples.append(sample)
                
        except Exception:
            samples.append(0)
    
    return np.array(samples, dtype=np.int16)

def upsample_linear(samples, ratio=12):
    """Linear interpolation upsampling (better quality than repeat)"""
    if len(samples) < 2:
        return np.repeat(samples, ratio)
    
    # Create time indices
    x_old = np.arange(len(samples))
    x_new = np.linspace(0, len(samples) - 1, len(samples) * ratio)
    
    # Linear interpolation
    upsampled = np.interp(x_new, x_old, samples)
    
    return upsampled.astype(np.int16)

def main():
    # Find serial port
    port = find_serial_port()
    if not port:
        print("❌ No port selected. Exiting.")
        return
    
    print(f"\n🔌 Opening {port} at {BAUD_RATE} baud...")
    
    try:
        # Open serial port with larger buffer
        ser = serial.Serial(
            port, 
            BAUD_RATE, 
            timeout=0.1,
            write_timeout=0.1
        )
        
        # Flush any old data
        ser.reset_input_buffer()
        
        print("✅ Connected! Starting audio playback...")
        print(f"📊 Sample rate: {SAMPLE_RATE} Hz → {SAMPLE_RATE * UPSAMPLE_RATIO} Hz")
        print("🎵 Press Ctrl+C to stop\n")
        
        # Create audio stream
        stream = sd.OutputStream(
            samplerate=SAMPLE_RATE * UPSAMPLE_RATIO,  # 48 kHz
            channels=1,
            dtype='int16',
            blocksize=BUFFER_SIZE * UPSAMPLE_RATIO,
            latency='low'
        )
        
        stream.start()
        
        # Statistics
        sample_count = 0
        error_count = 0
        
        # Main loop
        while True:
            # Receive samples at 4 kHz
            samples = receive_samples(ser, BUFFER_SIZE)
            
            if len(samples) == 0:
                error_count += 1
                continue
            
            # Upsample to 48 kHz using linear interpolation
            upsampled = upsample_linear(samples, ratio=UPSAMPLE_RATIO)
            
            # Play audio
            try:
                stream.write(upsampled)
                sample_count += len(samples)
            except Exception as e:
                error_count += 1
            
            # Print status (every 1000 samples)
            if sample_count % 1000 == 0:
                sys.stdout.write(
                    f"\r🎵 Samples: {sample_count:6d} | "
                    f"Errors: {error_count:3d} | "
                    f"Latency: {stream.latency*1000:.0f}ms   "
                )
                sys.stdout.flush()
    
    except KeyboardInterrupt:
        print("\n\n⏹️  Stopped by user")
    
    except serial.SerialException as e:
        print(f"\n❌ Serial error: {e}")
    
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
    
    finally:
        # Cleanup
        if 'stream' in locals():
            stream.stop()
            stream.close()
            print("🔇 Audio stream closed")
        
        if 'ser' in locals():
            ser.close()
            print("🔌 Serial port closed")
        
        print("\n✅ Done!")

if __name__ == "__main__":
    print("=" * 60)
    print("   MSPM0 Synthesizer - UART Audio Player (OPTIMIZED)")
    print("=" * 60)
    main()