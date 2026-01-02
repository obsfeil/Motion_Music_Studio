#!/usr/bin/env python3
"""
Auto-Detecting UART Receiver for MSPM0
Automatically detects if MIDI or raw audio is being sent

Usage:
    python auto_receiver.py         # Auto-detect
    python auto_receiver.py --midi  # Force MIDI mode
    python auto_receiver.py --audio # Force RAW audio mode
"""

import serial
import sounddevice as sd
import numpy as np
import sys
import struct
import argparse

# Configuration
BAUD_RATE = 921600
BUFFER_SIZE = 512

# MIDI Constants
MIDI_NOTE_OFF = 0x80
MIDI_NOTE_ON = 0x90
MIDI_CONTROL_CHANGE = 0xB0
MIDI_PROGRAM_CHANGE = 0xC0

def is_midi_byte(byte):
    """Check if byte looks like a MIDI status byte"""
    return byte >= 0x80 and byte <= 0xF7

def detect_protocol(ser):
    """Auto-detect if MIDI or raw audio is being sent"""
    print("🔍 Auto-detecting protocol...")
    print("   (Play a note or move controls to generate data...)")
    
    # Wait for data with timeout
    import time
    timeout = 3.0  # 3 second timeout
    start_time = time.time()
    test_bytes = []
    
    while (time.time() - start_time) < timeout and len(test_bytes) < 50:
        if ser.in_waiting > 0:
            data = ser.read(min(ser.in_waiting, 50))
            test_bytes.extend(data)
        else:
            time.sleep(0.01)
    
    if len(test_bytes) < 3:
        print("⚠️  Not enough data to detect protocol")
        print("   Checking if MIDI mode is configured in firmware...")
        # Default to MIDI if we see absolutely no data (likely MIDI mode)
        print("   → Defaulting to MIDI mode")
        return "midi"
    
    # MIDI detection logic
    # MIDI messages start with status byte (0x80-0xFF)
    # Followed by 1-2 data bytes (0x00-0x7F)
    midi_patterns = 0
    
    for i in range(len(test_bytes) - 2):
        # Check for MIDI Note On pattern: 0x9X XX XX
        if (test_bytes[i] & 0xF0) == MIDI_NOTE_ON:
            if (test_bytes[i+1] <= 0x7F) and (test_bytes[i+2] <= 0x7F):
                midi_patterns += 1
        
        # Check for MIDI Note Off pattern: 0x8X XX XX
        if (test_bytes[i] & 0xF0) == MIDI_NOTE_OFF:
            if (test_bytes[i+1] <= 0x7F) and (test_bytes[i+2] <= 0x7F):
                midi_patterns += 1
        
        # Check for MIDI CC pattern: 0xBX XX XX
        if (test_bytes[i] & 0xF0) == MIDI_CONTROL_CHANGE:
            if (test_bytes[i+1] <= 0x7F) and (test_bytes[i+2] <= 0x7F):
                midi_patterns += 1
    
    # Raw audio typically has high bytes set (16-bit samples)
    # MIDI data bytes never have bit 7 set
    audio_like = sum(1 for b in test_bytes[1::2] if b > 0x7F)  # Check high bytes
    
    print(f"   MIDI patterns found: {midi_patterns}")
    print(f"   Audio-like bytes: {audio_like}")
    
    if midi_patterns >= 2:
        print("✅ Detected: MIDI protocol")
        return "midi"
    elif audio_like > len(test_bytes) // 4:
        print("✅ Detected: RAW AUDIO protocol")
        return "audio"
    else:
        # If uncertain but we see some data, default to MIDI
        # (MIDI is more likely if firmware was recently updated)
        print("⚠️  Uncertain, defaulting to MIDI")
        return "midi"

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
    
    return ports[0].device

# ============================================================================
# RAW AUDIO MODE
# ============================================================================
class AudioReceiver:
    def __init__(self, sample_rate=4000):
        self.sample_rate = sample_rate
        self.upsample_ratio = 12  # 48kHz output
        
    def receive_samples(self, ser, num_samples):
        """Receive audio samples from UART"""
        samples = []
        for _ in range(num_samples):
            try:
                data = ser.read(2)
                if len(data) != 2:
                    samples.append(0)
                    continue
                sample = struct.unpack('<h', data)[0]
                if abs(sample) > 32000:
                    samples.append(0)
                else:
                    samples.append(sample)
            except Exception:
                samples.append(0)
        return np.array(samples, dtype=np.int16)
    
    def upsample(self, samples):
        """Linear interpolation upsampling"""
        if len(samples) < 2:
            return np.repeat(samples, self.upsample_ratio)
        x_old = np.arange(len(samples))
        x_new = np.linspace(0, len(samples) - 1, len(samples) * self.upsample_ratio)
        upsampled = np.interp(x_new, x_old, samples)
        return upsampled.astype(np.int16)
    
    def run(self, ser):
        """Run audio receiver"""
        stream = sd.OutputStream(
            samplerate=self.sample_rate * self.upsample_ratio,
            channels=1,
            dtype='int16',
            blocksize=BUFFER_SIZE * self.upsample_ratio,
            latency='low',
            device=27
        )
        stream.start()
        
        sample_count = 0
        print(f"🎵 RAW AUDIO Mode - {self.sample_rate} Hz → {self.sample_rate * self.upsample_ratio} Hz")
        
        try:
            while True:
                samples = self.receive_samples(ser, BUFFER_SIZE)
                if len(samples) > 0:
                    upsampled = self.upsample(samples)
                    stream.write(upsampled)
                    sample_count += len(samples)
                    
                    if sample_count % 1000 == 0:
                        sys.stdout.write(f"\r🎵 Samples: {sample_count:6d}   ")
                        sys.stdout.flush()
        finally:
            stream.stop()
            stream.close()

# ============================================================================
# MIDI MODE
# ============================================================================
class Voice:
    """Single synthesizer voice"""
    def __init__(self, sample_rate=48000):
        self.sample_rate = sample_rate
        self.note = 0
        self.frequency = 0
        self.velocity = 0
        self.phase = 0
        self.active = False
        self.envelope = 0.0
        self.envelope_state = 'release'
        
    def note_on(self, note, velocity):
        self.note = note
        self.frequency = 440.0 * (2.0 ** ((note - 69) / 12.0))
        self.velocity = velocity / 127.0
        self.active = True
        self.envelope_state = 'attack'
        self.envelope = 0.0
        
    def note_off(self):
        """Trigger note off - start release phase"""
        if self.envelope_state != 'release':
            self.envelope_state = 'release'
            # Store current envelope level for release
            self.release_start_level = self.envelope
        
    def generate(self, num_samples):
        if not self.active and self.envelope <= 0.0:
            return np.zeros(num_samples, dtype=np.float32)
        
        phase_increment = 2.0 * np.pi * self.frequency / self.sample_rate
        phases = self.phase + np.arange(num_samples) * phase_increment
        self.phase = phases[-1] % (2.0 * np.pi)
        
        # Sawtooth wave
        samples = (phases % (2.0 * np.pi)) / np.pi - 1.0
        
        # Simple ADSR
        attack_samples = int(0.01 * self.sample_rate)  # 10ms attack
        release_samples = int(0.05 * self.sample_rate)  # 50ms release (faster!)
        
        envelopes = np.zeros(num_samples, dtype=np.float32)
        for i in range(num_samples):
            if self.envelope_state == 'attack':
                self.envelope += 1.0 / attack_samples
                if self.envelope >= 1.0:
                    self.envelope = 1.0
                    self.envelope_state = 'sustain'
            elif self.envelope_state == 'sustain':
                self.envelope = 0.8
            elif self.envelope_state == 'release':
                # Release from current level, not from 0.8!
                if not hasattr(self, 'release_start_level'):
                    self.release_start_level = self.envelope
                self.envelope -= self.release_start_level / release_samples
                if self.envelope <= 0.0:
                    self.envelope = 0.0
                    self.active = False
            
            envelopes[i] = max(0.0, self.envelope)  # Clamp to 0
        
        return samples * envelopes * self.velocity * 0.3

class MIDISynthesizer:
    def __init__(self, sample_rate=48000, max_voices=8):
        self.sample_rate = sample_rate
        self.voices = [Voice(sample_rate) for _ in range(max_voices)]
        self.volume = 0.8
        
    def note_on(self, note, velocity):
        if velocity == 0:
            self.note_off(note)
            return
        voice = None
        for v in self.voices:
            if not v.active:
                voice = v
                break
        if voice is None:
            voice = self.voices[0]
        voice.note_on(note, velocity)
    
    def note_off(self, note):
        for v in self.voices:
            if v.active and v.note == note:
                v.note_off()
    
    def control_change(self, controller, value):
        if controller == 0x07:  # Volume
            self.volume = value / 127.0
        elif controller == 123:  # All Notes Off - PANIC!
            # Immediately kill all voices without release
            for voice in self.voices:
                voice.active = False
                voice.envelope = 0.0
                voice.envelope_state = 'release'
                voice.phase = 0.0
    
    def generate(self, num_samples):
        output = np.zeros(num_samples, dtype=np.float32)
        for voice in self.voices:
            output += voice.generate(num_samples)
        output *= self.volume
        return np.clip(output * 32767, -32768, 32767).astype(np.int16)

class MIDIReceiver:
    def __init__(self):
        self.synth = MIDISynthesizer()
        
    def parse_midi(self, data_buffer):
        if len(data_buffer) == 0:
            return None, 0
        
        status = data_buffer[0]
        if status >= 0x80 and status <= 0xEF:
            message_type = status & 0xF0
            channel = status & 0x0F
            
            if message_type in [MIDI_NOTE_OFF, MIDI_NOTE_ON, MIDI_CONTROL_CHANGE]:
                if len(data_buffer) >= 3:
                    return {
                        'type': 'note_on' if message_type == MIDI_NOTE_ON else 
                                ('note_off' if message_type == MIDI_NOTE_OFF else 'control_change'),
                        'channel': channel,
                        'data1': data_buffer[1],
                        'data2': data_buffer[2]
                    }, 3
            elif message_type == MIDI_PROGRAM_CHANGE:
                if len(data_buffer) >= 2:
                    return {'type': 'program_change', 'channel': channel, 'program': data_buffer[1]}, 2
        return None, 0
    
    def audio_callback(self, outdata, frames, time_info, status):
        audio = self.synth.generate(frames)
        outdata[:] = audio.reshape(-1, 1)
    
    def run(self, ser):
        stream = sd.OutputStream(
            samplerate=48000,
            channels=1,
            dtype='int16',
            blocksize=BUFFER_SIZE,
            callback=self.audio_callback,
            latency='low'
        )
        stream.start()
        
        midi_buffer = bytearray()
        note_count = 0
        print("🎹 MIDI Mode - 48 kHz polyphonic synth")
        
        try:
            while True:
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)
                    midi_buffer.extend(data)
                
                while len(midi_buffer) >= 3:
                    msg, consumed = self.parse_midi(midi_buffer)
                    if msg:
                        if msg['type'] == 'note_on':
                            self.synth.note_on(msg['data1'], msg['data2'])
                            note_count += 1
                            sys.stdout.write(f"\r🎹 Notes: {note_count:6d}   ")
                            sys.stdout.flush()
                        elif msg['type'] == 'note_off':
                            self.synth.note_off(msg['data1'])
                        elif msg['type'] == 'control_change':
                            self.synth.control_change(msg['data1'], msg['data2'])
                        midi_buffer = midi_buffer[consumed:]
                    else:
                        midi_buffer = midi_buffer[1:]
                
                sd.sleep(1)
        finally:
            stream.stop()
            stream.close()

# ============================================================================
# MAIN
# ============================================================================
def main():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='MSPM0 UART Receiver')
    parser.add_argument('--midi', action='store_true', help='Force MIDI mode')
    parser.add_argument('--audio', action='store_true', help='Force RAW audio mode')
    args = parser.parse_args()
    
    port = find_serial_port()
    if not port:
        print("❌ No port selected. Exiting.")
        return
    
    print(f"\n🔌 Opening {port} at {BAUD_RATE} baud...")
    
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=0.001)
        ser.reset_input_buffer()
        print("✅ Connected!")
        
        # Determine protocol
        if args.midi:
            protocol = "midi"
            print("🎹 Forced MIDI mode (--midi)")
        elif args.audio:
            protocol = "audio"
            print("🎵 Forced RAW AUDIO mode (--audio)")
        else:
            # Auto-detect protocol
            protocol = detect_protocol(ser)
            ser.reset_input_buffer()  # Clear test data
        
        print("\n🎵 Press Ctrl+C to stop\n")
        
        if protocol == "midi":
            receiver = MIDIReceiver()
            receiver.run(ser)
        else:
            receiver = AudioReceiver()
            receiver.run(ser)
    
    except KeyboardInterrupt:
        print("\n\n⏹️  Stopped by user")
    except serial.SerialException as e:
        print(f"\n❌ Serial error: {e}")
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        if 'ser' in locals():
            ser.close()
            print("🔌 Serial port closed")
        print("\n✅ Done!")

if __name__ == "__main__":
    print("=" * 60)
    print("   MSPM0 Auto-Detecting Receiver (MIDI/Audio)")
    print("=" * 60)
    main()