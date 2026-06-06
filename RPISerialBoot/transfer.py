#!/usr/bin/env python3

import serial
import struct
import sys
import os
import tty
import termios
import select
import threading

READY_BYTE = 0x03
ACK = b'OK'
BAUD = 115200
PORT = '/dev/tty.usbmodem142101'

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
                # Ctrl+A then X to exit, like minicom
                if prev == b'\x01' and ch == b'x':
                    break
                prev = ch
                ser.write(ch)
    finally:
        stop.set()
        termios.tcsetattr(fd, termios.TCSADRAIN, old)
        print("\n[Exited terminal mode]")

def send_kernel(port, kernel_path):
    with open(kernel_path, 'rb') as f:
        payload = f.read()

    size = len(payload)
    print(f"Sending {kernel_path} ({size} bytes)")

    with serial.Serial(port, BAUD, timeout=10) as ser:
        print("Waiting for bootloader ready signal...")
        ser.reset_input_buffer()
        ser.reset_output_buffer()

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
        if ack != ACK:
            print(f"Bad ACK: {ack!r}")
            sys.exit(1)

        print("Size acknowledged, sending payload...")

        chunk_size = 512
        sent = 0
        while sent < size:
            chunk = payload[sent:sent + chunk_size]
            ser.write(chunk)
            sent += len(chunk)
            percent = (sent / size) * 100
            print(f"\r  {sent}/{size} bytes ({percent:.1f}%)", end='', flush=True)

        print("\nPayload sent! Entering terminal mode...")

        # Drain any incoming bytes briefly
        import time
        time.sleep(0.5)
        while ser.in_waiting:
            sys.stdout.buffer.write(ser.read(ser.in_waiting))
            sys.stdout.buffer.flush()

        # Hand off to terminal mode, port stays open
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