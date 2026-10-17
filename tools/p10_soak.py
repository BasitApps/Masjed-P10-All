"""Read-only P10 serial/HTTP soak collector; never sends serial commands."""
import argparse
import ctypes
from datetime import datetime, timedelta, timezone
import json
import os
from pathlib import Path
import re
import time
import urllib.request
import serial

BOOT = re.compile(r'rst cause:\s*(\d+)', re.IGNORECASE)
OPENER = urllib.request.build_opener(urllib.request.ProxyHandler({}))


def parse_health(body):
    allowed = {'v', 'ok', 'profile', 'fw', 'config', 'reset', 'resetInfo',
               'uptime', 'heap', 'maxBlock', 'fragmentation', 'rtc', 'timeValid',
               'rssi', 'apClients', 'syncAge'}
    values = dict(line.split('=', 1) for line in body.decode('utf-8').splitlines() if '=' in line)
    if values.get('v') != '2' or values.get('ok') != '1' or values.get('profile') != '4x1':
        raise ValueError('Not a successful P10 4x1 health response')
    values = {key: value for key, value in values.items() if key in allowed}
    for key in ('uptime', 'heap', 'maxBlock', 'fragmentation', 'rtc', 'timeValid', 'apClients', 'syncAge', 'config'):
        if key in values:
            values[key] = int(values[key])
            if values[key] < 0:
                raise ValueError('Negative health counter')
    if 'uptime' not in values or 'heap' not in values:
        raise ValueError('Missing health counters')
    return values


def uptime_event(previous, current):
    if previous is not None and current < previous:
        # /health derives seconds from 32-bit millis(), not a 32-bit seconds counter.
        if previous >= 4294900 and current < 120:
            return 'possible_millis_wrap'
        return 'uptime_drop'
    return None


def open_serial(name, factory=serial.Serial):
    port = factory(port=None, baudrate=74880, timeout=1)
    port.dtr = False
    port.rts = False
    port.port = name
    try:
        port.open()
    except Exception:
        port.close()
        raise
    return port


def post(path):
    request = urllib.request.Request('http://192.168.1.177' + path, data=b'v=2',
                                    headers={'Content-Type': 'application/octet-stream'})
    with OPENER.open(request, timeout=2) as response:
        body = response.read(4097)
        if len(body) > 4096:
            raise ValueError('Oversize response')
        return body


def utc():
    return datetime.now(timezone.utc).isoformat()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--hours', type=float, default=12)
    parser.add_argument('--port', default='COM5')
    args = parser.parse_args()
    if not 0 < args.hours <= 24:
        parser.error('hours must be greater than 0 and at most 24')
    args.out.mkdir(parents=True, exist_ok=True)
    started = datetime.now(timezone.utc)
    start = time.monotonic()
    end = start + args.hours * 3600
    state = dict(state='running', pid=os.getpid(), started_utc=started.isoformat(),
                 deadline_utc=(started + timedelta(hours=args.hours)).isoformat(),
                 port=args.port, serial_connected=False, health_samples=0,
                 health_failures=0, consecutive_health_failures=0, boot_markers=0,
                 uptime_drops=0, possible_wraps=0, serial_errors=0, coverage_gaps=0,
                 rtc_bad_samples=0, selftest_samples=0, selftest_failures=0,
                 expected_bin_sha256='F3D0A4677CC9AA48AC96FAD400B5C571B7435121B2F04D7A8B71EF074C5C9A83')
    port = None
    pending = b''
    last_iteration = start
    next_open = next_health = next_selftest = next_status = start
    opened_at = None
    previous_uptime = None
    serial_error = None
    keep_awake = None
    if os.name == 'nt':
        keep_awake = ctypes.windll.kernel32.SetThreadExecutionState
        state['sleep_inhibition_active'] = bool(keep_awake(0x80000001))
    with (args.out / 'events.jsonl').open('a', encoding='utf-8', buffering=1) as events, \
            (args.out / 'serial.raw').open('ab', buffering=0) as raw:
        def event(kind, **data):
            entry = dict(utc=utc(), kind=kind, **data)
            events.write(json.dumps(entry, ensure_ascii=False) + '\n')
            state['last_event'] = entry

        def status():
            state['updated_utc'] = utc()
            state['elapsed_seconds'] = round(time.monotonic() - start, 1)
            state['remaining_seconds'] = max(0, round(end - time.monotonic(), 1))
            tmp = args.out / 'status.tmp'
            tmp.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding='utf-8')
            os.replace(tmp, args.out / 'status.json')

        event('started', port=args.port, hours=args.hours, no_device_writes=True)
        status()
        try:
            while time.monotonic() < end:
                now = time.monotonic()
                if now - last_iteration > 10:
                    state['coverage_gaps'] += 1
                    event('observation_gap', seconds=round(now - last_iteration, 1))
                last_iteration = now
                if now >= next_health:
                    next_health = now + 30
                    try:
                        health = parse_health(post('/health'))
                        hint = uptime_event(previous_uptime, health['uptime'])
                        if hint:
                            state['uptime_drops' if hint == 'uptime_drop' else 'possible_wraps'] += 1
                            event(hint, previous=previous_uptime, current=health['uptime'],
                                  within_serial_open_window=opened_at is not None and now - opened_at < 15)
                        previous_uptime = health['uptime']
                        state['last_health'] = health
                        state['last_health_utc'] = utc()
                        state['health_samples'] += 1
                        state['consecutive_health_failures'] = 0
                        state['minimum_heap'] = min(state.get('minimum_heap', health['heap']), health['heap'])
                        state['maximum_fragmentation'] = max(state.get('maximum_fragmentation', 0), health.get('fragmentation', 0))
                        if health.get('rtc') != 1 or health.get('timeValid') != 1:
                            state['rtc_bad_samples'] += 1
                        event('health', **health)
                    except Exception as error:
                        state['health_failures'] += 1
                        state['consecutive_health_failures'] += 1
                        event('health_unavailable', error=str(error))
                if now >= next_selftest:
                    next_selftest = now + 300
                    try:
                        body = post('/selfTest').decode('utf-8')
                        fields = dict(line.split('=', 1) for line in body.splitlines() if '=' in line)
                        if fields.get('v') != '2' or fields.get('ok') != '1' or fields.get('profile') != '4x1':
                            raise ValueError('selfTest did not report healthy P10')
                        state['selftest_samples'] += 1
                        event('selftest', ok=True)
                    except Exception as error:
                        state['selftest_failures'] += 1
                        event('selftest_unavailable', error=str(error))
                if port is None and now >= next_open:
                    next_open = now + 5
                    try:
                        port = open_serial(args.port)
                        opened_at = time.monotonic()
                        state['serial_connected'] = True
                        serial_error = None
                        pending = b''
                        event('serial_opened', dtr=False, rts=False)
                    except Exception as error:
                        if str(error) != serial_error:
                            serial_error = str(error)
                            state['serial_errors'] += 1
                            event('serial_unavailable', error=serial_error)
                if port is not None:
                    try:
                        chunk = port.read(2048)
                        if chunk:
                            raw.write(chunk)
                            pending += chunk
                            lines = pending.split(b'\n')
                            pending = lines.pop()[-4096:]
                            for line in lines:
                                text = line.decode('utf-8', errors='replace').strip()
                                match = BOOT.search(text)
                                if match:
                                    state['boot_markers'] += 1
                                    event('rom_boot', cause=int(match.group(1)), text=text[:256],
                                          within_serial_open_window=time.monotonic() - opened_at < 15)
                    except Exception as error:
                        state['serial_errors'] += 1
                        state['serial_connected'] = False
                        event('serial_lost', error=str(error))
                        port.close()
                        port = None
                else:
                    time.sleep(1)
                if time.monotonic() >= next_status:
                    next_status = time.monotonic() + 30
                    status()
            state['state'] = 'completed'
            evidence = ('boot_markers', 'uptime_drops', 'possible_wraps', 'serial_errors',
                        'coverage_gaps', 'health_failures', 'selftest_failures', 'rtc_bad_samples')
            state['result'] = 'review_required' if not state['health_samples'] or any(state[key] for key in evidence) else 'no_reset_observed'
            event('completed', result=state['result'])
        except BaseException as error:
            state['state'] = 'failed'
            event('collector_failed', error=repr(error))
            raise
        finally:
            if port is not None:
                port.close()
            state['serial_connected'] = False
            if keep_awake:
                keep_awake(0x80000000)
                state['sleep_inhibition_active'] = False
            status()


if __name__ == '__main__':
    main()
