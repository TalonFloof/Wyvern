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

        response = ser.read(1)
        if not response:
            print(f"\nTimeout on chunk {chunk_num}/{total_chunks}")
            continue
        if response[0] == ACK:
            return True
        elif response[0] == NAK:
            print(f"\nNAK on chunk {chunk_num}/{total_chunks}, "
                  f"retrying (attempt {attempt + 1}/{MAX_RETRIES})")
            continue
        else:
            print(f"\nUnexpected response: {hex(response[0])}")
            continue

    print(f"\nMax retries exceeded on chunk {chunk_num}/{total_chunks}")
    return False

def send_kernel(port, kernel_path):
    with open(kernel_path, 'rb') as f:
        payload = f.read()

    size = len(payload)
    chunks = [payload[i:i + CHUNK_SIZE]
              for i in range(0, size, CHUNK_SIZE)]
    total_chunks = len(chunks)

    print(f"Sending {kernel_path} ({size} bytes, "
          f"{total_chunks} chunks of {CHUNK_SIZE} bytes)")

    ser = serial.Serial()
    ser.port = PORT
    ser.baudrate = BAUD
    ser.timeout = 10
    ser.rts = False
    ser.dtr = False        # prevent reset before opening
    ser.open()

    import time
    time.sleep(2)
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    print("Waiting for bootloader ready signal...")

    ready_count = 0
    while ready_count < 3:
        byte = ser.read(1)
        if not byte:
            print("Timeout waiting for ready signal")
            sys.exit(1)
        if byte[0] == READY_BYTE:
            ready_count += 1
        else:
            ready_count = 0

    print("Bootloader ready, sending size...")
    ser.write(struct.pack('<I', size))

    ack = ser.read(2)
    if ack != b'OK':
        print(f"Bad ACK: {ack!r}")
        sys.exit(1)

    print("Size acknowledged, sending chunks...")

    for i, chunk in enumerate(chunks):
        chunk_num = i + 1
        percent = (chunk_num / total_chunks) * 100
        print(f"\r  Chunk {chunk_num}/{total_chunks} "
                f"({len(chunk)} bytes) {percent:.1f}%", end='', flush=True)

        if not send_chunk(ser, chunk, chunk_num, total_chunks):
            print("\nTransfer failed")
            sys.exit(1)

    print("\nAll chunks received!")

    # Wait for GO
    response = ser.read(2)
    if response == b'GO':
        print("Bootloader confirmed, entering terminal mode...")
    else:
        print(f"Unexpected final response: {response!r}")
        sys.exit(1)

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