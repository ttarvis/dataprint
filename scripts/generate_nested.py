#!/usr/bin/env python3
import json
import random
import string
import sys

def random_string(length=8):
    return ''.join(random.choices(string.ascii_letters, k=length))

def random_value(depth=0):
    if depth > 3:
        # at max depth return primitives only
        return random.choice([
            random.randint(0, 1000000),
            random.uniform(0, 1000),
            random_string(random.randint(4, 16)),
            random.choice([True, False]),
            None
        ])
    
    r = random.random()
    if r < 0.3:
        # nested object
        return {random_string(6): random_value(depth + 1) 
                for _ in range(random.randint(3, 8))}
    elif r < 0.5:
        # array of values
        return [random_value(depth + 1) 
                for _ in range(random.randint(2, 6))]
    else:
        # primitive
        return random.choice([
            random.randint(0, 1000000),
            random.uniform(0, 1000),
            random_string(random.randint(4, 16)),
            random.choice([True, False]),
            None
        ])

def generate_nested_record():
    return {
        "id": random.randint(1, 10000000),
        "timestamp": random.randint(1600000000, 1700000000),
        "user": {
            "id": random.randint(1, 1000000),
            "name": random_string(10),
            "email": f"{random_string(8)}@{random_string(6)}.com",
            "preferences": {
                random_string(6): random_value(1)
                for _ in range(random.randint(3, 8))
            }
        },
        "session": {
            "id": random_string(16),
            "duration": random.randint(1, 7200),
            "events": [
                {
                    "type": random_string(6),
                    "timestamp": random.randint(1600000000, 1700000000),
                    "data": {
                        random_string(5): random_value(2)
                        for _ in range(random.randint(2, 5))
                    }
                }
                for _ in range(random.randint(2, 8))
            ]
        },
        "metadata": {
            random_string(6): random_value(1)
            for _ in range(random.randint(5, 15))
        }
    }

def main():
    if len(sys.argv) != 3:
        print("Usage: generate_nested.py <output_file> <size_gb>")
        sys.exit(1)

    output_file = sys.argv[1]
    target_bytes = int(float(sys.argv[2]) * 1024 * 1024 * 1024)

    written = 0
    with open(output_file, 'w') as f:
        while written < target_bytes:
            line = json.dumps(generate_nested_record()) + '\n'
            f.write(line)
            written += len(line.encode('utf-8'))

    print(f"Written {written / (1024**3):.2f} GB to {output_file}")

if __name__ == '__main__':
    main()
