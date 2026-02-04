#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/cu.usbserial-0001}"
BAUD="${2:-115200}"

python - "$PORT" "$BAUD" <<'PY'
import sys
import time

try:
    import serial
except Exception as exc:
    print("pyserial required: pip install pyserial", file=sys.stderr)
    raise SystemExit(1) from exc

port = sys.argv[1]
baud = int(sys.argv[2])
ser = serial.Serial(port, baud, timeout=0.1)

count = 0
start = time.time()
last = start

try:
    while True:
        line = ser.readline()
        if line:
            count += 1
        now = time.time()
        if now - last >= 1.0:
            rate = count / max(1e-6, now - start)
            print(f"{rate:0.1f} values/sec")
            last = now
except KeyboardInterrupt:
    pass
finally:
    ser.close()
PY
