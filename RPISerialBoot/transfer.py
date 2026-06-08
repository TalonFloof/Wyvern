#!/usr/bin/env python3

import serial
import struct
import sys
import tty
import termios
import select
import threading
import time
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

        if response[0] == ACK:
            if echoed_num != chunk_num:
                print(f"\nChunk number mismatch on ACK: "
                      f"sent {chunk_num} got {echoed_num}")
                continue
            return True
        elif response[0] == NAK:
            if echoed_num != chunk_num:
                print(f"\nChunk number mismatch on NAK: "
                      f"sent {chunk_num} got {echoed_num}")
                continue
            print(f"\nNAK on chunk {chunk_num}/{total_chunks}, "
                  f"retrying (attempt {attempt + 1}/{MAX_RETRIES})")
            continue
        else:
            # Unexpected response byte — the bootloader likely sent a RES
            # signal because it timed out. Return False immediately without
            # retrying: each retry sends 516 more bytes of stale data into
            # the UART, which the bootloader cannot flush before resuming
            # and which will cause CRC failures on every subsequent chunk.
            print(f"\nUnexpected response 0x{response[0]:02X} on chunk "
                  f"{chunk_num}/{total_chunks}, stopping retries")
            return False

    print(f"\nMax retries exceeded on chunk {chunk_num}/{total_chunks}")
    return False

def wait_for_signal(ser, size, total_chunks):
    """Slide a 3-byte window over incoming bytes looking for either:
      - three READY_BYTEs  → fresh transfer: send size, get OK, return 0
      - b'RES' + uint32_le → resume: send OK, return next chunk index

    Handles both the initial handshake and any mid-transfer reconnect,
    including the case where the bootloader resets to fresh-start after
    NAK exhaustion."""
    buf = bytearray()
    while True:
        b = ser.read(1)
        if not b:
            print("Timeout waiting for bootloader signal")
            sys.exit(1)
        buf.append(b[0])
        # Maintain a true sliding window of the last 3 bytes
        if len(buf) > 3:
            del buf[0]
        if len(buf) < 3:
            continue

        if buf == bytearray(b'RES'):
            chunk_bytes = ser.read(4)
            if len(chunk_bytes) < 4:
                print("Timeout reading resume chunk number")
                sys.exit(1)
            last_good = struct.unpack('<I', chunk_bytes)[0]
            next_chunk = last_good + 1
            print(f"\nResume from chunk {next_chunk}/{total_chunks}")
            # Echo the chunk number back so the bootloader can verify this
            # 'OK' isn't a false match from stale binary data in the FIFO.
            ser.write(b'OK')
            ser.write(struct.pack('<I', last_good))
            # Wait for the bootloader's ready signal, which it only sends
            # after flushing all stale bytes — guarantees a clean FIFO
            # before we start sending chunk data again.
            ready = ser.read(1)
            if not ready or ready[0] != ACK:
                print(f"Unexpected resume-ready signal: {ready!r}")
                sys.exit(1)
            return next_chunk

        elif all(c == READY_BYTE for c in buf):
            print("Bootloader ready, sending size...")
            ser.write(struct.pack('<I', size))
            ack = ser.read(2)
            if ack != b'OK':
                print(f"Bad size ACK: {ack!r}")
                sys.exit(1)
            return 0

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

    time.sleep(2.0)
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    print("Waiting for bootloader signal...")
    i = wait_for_signal(ser, size, total_chunks)

    print(f"Sending from chunk {i}/{total_chunks}...")

    while i < total_chunks:
        chunk = chunks[i]
        percent = ((i + 1) / total_chunks) * 100
        print(f"\r  Chunk {i + 1}/{total_chunks} "
              f"({len(chunk)} bytes) {percent:.1f}%", end='', flush=True)

        try:
            if send_chunk(ser, chunk, i, total_chunks):
                i += 1
            else:
                # Retries exhausted — wait for whatever signal the bootloader
                # sends next (could be RES for resume or READY for fresh start)
                print(f"\nRetries exhausted on chunk {i}, "
                      f"waiting for bootloader signal...")
                i = wait_for_signal(ser, size, total_chunks)
        except serial.SerialException:
            print(f"\nConnection lost on chunk {i + 1}, "
                  f"waiting for bootloader signal...")
            i = wait_for_signal(ser, size, total_chunks)

    print("\nAll chunks sent!")

    response = ser.read(2)
    if response == b'GO':
        print("Transfer complete, entering terminal mode...")
    else:
        print(f"Unexpected response: {response!r}")
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
