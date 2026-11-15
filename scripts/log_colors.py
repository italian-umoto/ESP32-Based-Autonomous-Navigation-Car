#!/usr/bin/env python3
"""Log the bot's color telemetry to a CSV file.

Three ways to get the data:

  Course server (default): connects to the course WebSocket server as a
  client and records the bot's telemetry as the server relays it. No
  firmware changes needed.
      python3 log_colors.py -o run1.csv
      python3 log_colors.py --url ws://10.0.0.95:80/ws --id MSMOKETEST01 -o run1.csv

  Laptop as server: this script acts as the server the bot connects to.
  Point SERVER_IP / SERVER_PORT in websocket.cpp at this laptop.
      python3 log_colors.py --listen 8080 -o run1.csv

  USB serial: reads the same lines the bot prints to Serial.
      python3 log_colors.py --serial /dev/ttyACM0 -o run1.csv

Stop with Ctrl-C; every row is flushed as it arrives, so nothing is lost.

Requires: pip install websockets pyserial
"""

import argparse
import asyncio
import csv
import json
import sys
import time

CLIENT_ID = "MAGICSMOKE67"  # the bot; telemetry lines start with this
AUTH_ID = "MSMOKETEST01"  # this laptop's id on the course server
DEFAULT_URL = "ws://10.5.9.24:80/ws"
PREFIX = f"{CLIENT_ID} color: "
COLUMNS = [
    "host_s",
    "bot_ms",
    "state",
    "L_r",
    "L_g",
    "L_b",
    "L_color",
    "R_r",
    "R_g",
    "R_b",
    "R_color",
]


class CsvSink:
    def __init__(self, path):
        self.f = open(path, "w", newline="")
        self.w = csv.writer(self.f)
        self.w.writerow(COLUMNS)
        self.t0 = time.monotonic()
        self.rows = 0

    def handle(self, line):
        """Write one telemetry line; ignore anything else."""
        line = line.strip()
        if not line.startswith(PREFIX):
            return
        fields = line[len(PREFIX) :].split(",")
        if len(fields) != len(COLUMNS) - 1:
            print(f"skipping malformed line: {line}", file=sys.stderr)
            return
        self.w.writerow([f"{time.monotonic() - self.t0:.3f}", *fields])
        self.f.flush()
        self.rows += 1
        if self.rows % 10 == 0:  # about once a second
            print(
                f"\r{self.rows} rows  last: {','.join(fields)}   ", end="", flush=True
            )

    def close(self):
        self.f.close()
        print(f"\nwrote {self.rows} rows")


def unwrap(msg):
    """The server may relay text as JSON ({"from": ..., "echo": ...}) or as-is."""
    if isinstance(msg, bytes):
        msg = msg.decode(errors="replace")
    try:
        doc = json.loads(msg)
    except ValueError:
        return msg
    if isinstance(doc, dict):
        for key in ("echo", "message", "msg", "data", "text"):
            if isinstance(doc.get(key), str):
                return doc[key]
    return msg


async def run_client(sink, url, client_id):
    from websockets.asyncio.client import connect

    shown = 0
    # connect() as an async iterator reconnects automatically if dropped
    async for ws in connect(url, open_timeout=10, ping_interval=20):
        try:
            print(f"connected to {url}, sending id {client_id}")
            await ws.send(client_id)
            authed = False
            async for msg in ws:
                text = unwrap(msg)
                if not authed and isinstance(msg, str):
                    if '"error"' in msg:
                        raise SystemExit(f"authentication failed: {msg}")
                    if '"authenticated"' in msg and '"ok"' in msg:
                        authed = True
                        print("authenticated, waiting for bot telemetry...")
                        continue
                if text.strip().startswith(PREFIX):
                    sink.handle(text)
                elif shown < 5:
                    # show the first few non-telemetry messages, to see
                    # what the server sends (auth replies, relay format)
                    print(f"server: {msg if isinstance(msg, str) else msg!r}")
                    shown += 1
        except Exception as e:  # dropped connection: log and reconnect
            print(f"\nconnection lost ({e}), reconnecting...")


async def run_server(sink, port):
    from websockets.asyncio.server import serve

    async def handler(ws):
        print(f"bot connected from {ws.remote_address[0]}")
        async for msg in ws:
            if isinstance(msg, bytes):
                msg = msg.decode(errors="replace")
            if msg.strip() == CLIENT_ID:
                # websocket.cpp sets authenticated once a reply contains
                # both "authenticated" and "ok"; telemetry is only sent after
                await ws.send('{"authenticated": "ok"}')
                print("bot authenticated, logging...")
                continue
            sink.handle(msg)
        print("\nbot disconnected (it will reconnect on its own)")

    async with serve(handler, "0.0.0.0", port):
        print(f"listening on ws://0.0.0.0:{port}/ws")
        await asyncio.Future()  # run until Ctrl-C


def run_serial(sink, device, baud):
    import serial

    with serial.Serial(device, baud, timeout=1) as port:
        print(f"reading {device} at {baud} baud, logging...")
        while True:
            sink.handle(port.readline().decode(errors="replace"))


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("-o", "--out", default="colors.csv", help="CSV file to write")
    ap.add_argument("--url", default=DEFAULT_URL, help="course server URL")
    ap.add_argument(
        "--id", default=AUTH_ID, help="approved id this laptop sends when it connects"
    )
    ap.add_argument(
        "--listen", type=int, metavar="PORT", help="act as the server on PORT instead"
    )
    ap.add_argument("--serial", metavar="DEVICE", help="read USB serial instead")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args()

    sink = CsvSink(args.out)
    try:
        if args.serial:
            run_serial(sink, args.serial, args.baud)
        elif args.listen:
            asyncio.run(run_server(sink, args.listen))
        else:
            asyncio.run(run_client(sink, args.url, args.id))
    except KeyboardInterrupt:
        pass
    finally:
        sink.close()


if __name__ == "__main__":
    main()
