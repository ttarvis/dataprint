#!/usr/bin/env python3
import random
import string
import sys


def random_string(length=8):
    return ''.join(random.choices(string.ascii_letters, k=length))


def generate_flat_record():
    name = random_string(random.randint(4, 12))
    year = random.randint(2010, 2024)
    score = random.randint(0, 100)
    completed = 'true' if random.random() > 0.5 else 'false'
    return f'["{name}", "{year}", {score}, {completed}]\n'


def main():
    if len(sys.argv) != 3:
        print("Usage: generate_flat.py <output_file> <size_gb>")
        sys.exit(1)

    output_file = sys.argv[1]
    target_bytes = int(float(sys.argv[2]) * 1024 * 1024 * 1024)

    written = 0
    buffer = []
    buffer_size = 0
    flush_threshold = 64 * 1024 * 1024  # flush every 64MB

    with open(output_file, 'w', buffering=8 * 1024 * 1024) as f:
        while written < target_bytes:
            line = generate_flat_record()
            buffer.append(line)
            buffer_size += len(line)

            if buffer_size >= flush_threshold:
                chunk = ''.join(buffer)
                f.write(chunk)
                written += buffer_size
                buffer = []
                buffer_size = 0

                gb_written = written / (1024 ** 3)
                target_gb = target_bytes / (1024 ** 3)
                print(f"\r{gb_written:.2f} / {target_gb:.1f} GB written", end='', flush=True)

        if buffer:
            chunk = ''.join(buffer)
            f.write(chunk)
            written += buffer_size

    print(f"\nDone. Written {written / (1024 ** 3):.2f} GB to {output_file}")


if __name__ == '__main__':
    main()
