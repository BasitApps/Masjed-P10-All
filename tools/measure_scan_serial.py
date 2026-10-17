"""COM5/74880 comparison for a scan-diagnostics build; restores AP/HTTP on exit."""
import re
import sys
import time
import serial

sys.stdout.reconfigure(encoding='utf-8', errors='replace')
port = serial.Serial(port=None, baudrate=74880, timeout=1)
port.dtr = False
port.rts = False
port.port = 'COM5'
port.open()
phase = 'wifi_on'
on_samples = off_samples = 0
disabled = False
deadline = time.monotonic() + 160
try:
    print('MEASURE_BEGIN: wait for peer; four connected WiFi-on windows, one transitional off window, three stable off windows', flush=True)
    while time.monotonic() < deadline:
        line = port.readline().decode('utf-8', errors='replace').strip()
        if not line:
            continue
        print(line, flush=True)
        match = re.search(r'^SCAN wifi=(\d+) peers=(\d+)', line)
        if not match:
            continue
        wifi_mode = int(match.group(1))
        peers = int(match.group(2))
        if phase == 'wifi_on' and wifi_mode in (2, 3) and peers > 0:
            on_samples += 1
            if on_samples == 4:
                port.write(b'W'); port.flush()
                disabled = True
                phase = 'wifi_off'
                print('SENT_W: next off window is transitional', flush=True)
        elif phase == 'wifi_off' and wifi_mode == 0:
            off_samples += 1
            print('OFF_WINDOW_TRANSITIONAL' if off_samples == 1 else 'OFF_WINDOW_STABLE', flush=True)
            if off_samples == 4:
                port.write(b'A'); port.flush()
                phase = 'restore'
                print('SENT_A: restoring AP and HTTP', flush=True)
        elif phase == 'restore' and wifi_mode in (2, 3):
            disabled = False
            print('MEASURE_DONE: AP restored', flush=True)
            break
    else:
        raise RuntimeError('Timed out before a complete comparison')
finally:
    if disabled:
        port.write(b'A'); port.flush()
        print('CLEANUP_A: AP/HTTP restore requested', flush=True)
        until = time.monotonic() + 5
        while time.monotonic() < until:
            line = port.readline().decode('utf-8', errors='replace').strip()
            if line:
                print(line, flush=True)
    port.close()
