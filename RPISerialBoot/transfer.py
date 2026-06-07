#!/usr/bin/env python3

import serial
import struct
import sys
import os
import tty
import termios
import select
import threading
import zlib

READY_BYTE = 0x03
ACK         = 0x06
NAK         = 0x15
BAUD = 115200
PORT = '/dev/ttyACM0'
CHUNK_SIZE = 512
MAX_RETRIES = 3

def crc32(data):
    return zlib.crc32(data) & 0xFFFFFFFF

def terminal_mode(ser):
    print("\n[Terminal mode active — Ctrl+A then X to exit]\n")
    fd = sys.stdin.fileno()
    old = termios.tcgetattr(fd)
    
    stop = threading.Event()

    def reader():
        while not stop.is_set():
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()

    t = threading.Thread(target=reader, daemon=True)
    t.start()

    try:
        tty.setraw(fd)
        prev = None
        while True:
            r, _, _ = select.select([sys.stdin], [], [], 0.1)
            if r:
                ch = sys.stdin.buffer.read(1)
                if ch == b'\x01':
                    prev = ch
                    continue
                if prev == b'\x01':
                    if ch == b'x':
                        break
                    else:
                        ser.write(prev)
                        ser.write(ch)
                else:
                    ser.write(ch)
                prev = ch
    finally:
        stop.set()
        termios.tcsetattr(fd, termios.TCSADRAIN, old)
        print("\n[Exited terminal mode]")

def send_chunk(ser, chunk, chunk_num, total_chunks):
    for attempt in range(MAX_RETRIES):
        ser.write(chunk)
        ser.write(struct.pack('<I', crc32(chunk)))

        # Read response byte + 4 byte chunk number
        response = ser.read(1)
        if not response:
            print(f"\nTimeout on chunk {chunk_num}/{total_chunks}")
            continue

        # Read echoed chunk number
        num_bytes = ser.read(4)
        if len(num_bytes) < 4:
            print(f"\nTimeout reading chunk number echo")
            continue

        echoed_num = struct.unpack('<I', num_bytes)[0]

        # Verify the echoed chunk number matches what we sent
        if echoed_num != chunk_num:
            print(f"\nChunk number mismatch: sent {chunk_num} "
                  f"got {echoed_num}")
            continue

        if response[0] == ACK:
            return True
        elif response[0] == NAK:
            print(f"\nNAK on chunk {chunk_num}/{total_chunks}, "
                  f"retrying (attempt {attempt + 1}/{MAX_RETRIES})")
            continue

    print(f"\nMax retries exceeded on chunk {chunk_num}/{total_chunks}")
    return False

def wait_for_resume(ser, total_chunks):
    """Wait for Pi resume signal, return next chunk index to send."""
    import time
    time.sleep(0.5)          # let line settle
    ser.reset_input_buffer() # flush stale bytes

    print("\nWaiting for resume signal...")
    buf = bytearray()

    while True:
        b = ser.read(1)
        if not b:
            print("Timeout waiting for resume signal")
            sys.exit(1)
        buf.append(b[0])

        # Look for RES marker in last 3 bytes
        if len(buf) >= 3 and buf[-3:] == bytearray(b'RES'):
            # Read 32-bit chunk number
            chunk_bytes = ser.read(4)
            if len(chunk_bytes) < 4:
                print("Timeout reading resume chunk number")
                sys.exit(1)
            last_good = struct.unpack('<I', chunk_bytes)[0]
            next_chunk = last_good + 1
            print(f"Resuming from chunk {next_chunk}/{total_chunks}")
            ser.write(b'OK')
            return next_chunk

        # Keep buffer from growing indefinitely
        if len(buf) > 16:
            buf = buf[-16:]

def send_kernel(port, kernel_path):
    with open(kernel_path, 'rb') as f:
        payload = f.read()

    size = len(payload)
    chunks = [payload[i:i + CHUNK_SIZE]
              for i in range(0, size, CHUNK_SIZE)]
    total_chunks = len(chunks)

    print(f"Sending {kernel_path} ({size} bytes, {total_chunks} chunks)")

    ser = serial.Serial()
    ser.port = port
    ser.baudrate = BAUD
    ser.timeout = 10
    ser.dtr = False
    ser.rts = False
    ser.open()

    import time
    time.sleep(2.0)
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    start_chunk = 0

    # Handshake loop — handles both fresh start and resume
    print("Waiting for signal...")
    while True:
        signal = []
        while len(signal) < 3:
            b = ser.read(1)
            if not b:
                print("Timeout waiting for signal")
                sys.exit(1)
            signal.append(b[0])
            if len(signal) > 3:
                signal.pop(0)

        if signal == [ord('R'), ord('E'), ord('S')]:
            # Resume signal — read 32-bit last good chunk number
            chunk_bytes = ser.read(4)
            if len(chunk_bytes) < 4:
                print("Timeout reading resume chunk number")
                sys.exit(1)
            last_good = struct.unpack('<I', chunk_bytes)[0]
            start_chunk = last_good + 1
            print(f"\nResume requested from chunk "
                  f"{start_chunk}/{total_chunks}")
            ser.write(b'OK')
            break

        elif all(b == READY_BYTE for b in signal):
            # Fresh transfer
            print("Bootloader ready, sending size...")
            ser.write(struct.pack('<I', size))

            ack = ser.read(2)
            if ack != b'OK':
                print(f"Bad ACK: {ack!r}")
                sys.exit(1)
            start_chunk = 0
            break

    # Send chunks using send_chunk
    print(f"Sending from chunk {start_chunk}/{total_chunks}...")

    i = start_chunk
    while i < total_chunks:
        chunk = chunks[i]
        percent = ((i + 1) / total_chunks) * 100
        print(f"\r  Chunk {i + 1}/{total_chunks} "
              f"({len(chunk)} bytes) {percent:.1f}%", end='', flush=True)

        try:
            if send_chunk(ser, chunk, i, total_chunks):
                i += 1
            else:
                i = wait_for_resume(ser, total_chunks)
        except serial.SerialException:
            print(f"\nConnection lost on chunk {i + 1}, "
                  f"waiting for resume signal...")
            i = wait_for_resume(ser, total_chunks)

    print("\nAll chunks sent!")

    response = ser.read(2)
    if response == b'GO':
        print("Transfer complete, entering terminal mode...")
    else:
        print(f"Unexpected response: {response!r}")
        sys.exit(1)

    import time
    time.sleep(0.5)
    while ser.in_waiting:
        sys.stdout.buffer.write(ser.read(ser.in_waiting))
        sys.stdout.buffer.flush()

    terminal_mode(ser)

def main():
    if len(sys.argv) == 1:
        # No file argument — just terminal mode
        print(f"Opening terminal on {PORT} at {BAUD} baud")
        with serial.Serial(PORT, BAUD, timeout=None) as ser:
            terminal_mode(ser)
    elif len(sys.argv) == 2:
        send_kernel(PORT, sys.argv[1])
    else:
        print(f"Usage: {sys.argv[0]} [kernel.img]")
        sys.exit(1)

if __name__ == '__main__':
    main()