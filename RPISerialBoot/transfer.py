#!/usr/bin/env python3

import serial
import struct
import sys
import time

READY_BYTE = 0x03
ACK = b'OK'
BAUD = 115200
PORT = '/dev/ttyACM0'

def send_kernel(port, kernel_path):
    with open(kernel_path, 'rb') as f:
        payload = f.read()

    size = len(payload)
    print(f"Sending {kernel_path} ({size} bytes)")

    with serial.Serial(port, BAUD, timeout=10) as ser:
        print("Waiting for bootloader ready signal...")

        # Wait for three consecutive 0x03 bytes
        ready_count = 0
        while ready_count < 3:
            byte = ser.read(1)
            if not byte:
                print("Timeout waiting for ready signal")
                sys.exit(1)
            if byte[0] == READY_BYTE:
                ready_count += 1
            else:
                # Reset count on any non-ready byte
                # Could be leftover UART output
                ready_count = 0

        print("Bootloader ready, sending size...")

        # Send payload size as 4 byte little-endian
        ser.write(struct.pack('<I', size))

        # Wait for OK acknowledgement
        ack = ser.read(2)
        if ack != ACK:
            print(f"Bad ACK: {ack!r}")
            sys.exit(1)

        print("Size acknowledged, sending payload...")

        # Send in chunks so we can show progress
        chunk_size = 512
        sent = 0
        while sent < size:
            chunk = payload[sent:sent + chunk_size]
            ser.write(chunk)
            sent += len(chunk)
            percent = (sent / size) * 100
            print(f"\r  {sent}/{size} bytes ({percent:.1f}%)", end='', flush=True)

        print("\nPayload sent")

        print("Bootloader response:")
        time.sleep(0.5)
        while ser.in_waiting:
            print(ser.read(ser.in_waiting).decode('utf-8', errors='replace'), end='')

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <kernel.img>")
        sys.exit(1)

    send_kernel(PORT, sys.argv[1])

if __name__ == '__main__':
    main()