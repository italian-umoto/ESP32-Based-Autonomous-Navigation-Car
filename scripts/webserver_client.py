#!/usr/bin/env python3
import asyncio
import sys

import websockets

AUTH_ID    = "MSMOKETEST01"
#AUTH_ID    = "MAGICSMOKE67"
CMD_PREFIX = "MAGICSMOKE67"
DEFAULT_URL = "ws://10.5.9.24:80/ws"
#DEFAULT_URL = "ws://10.0.0.95:80/ws"
NUM_STATES = 7


def build_command(line: str):
    parts = [p.strip() for p in line.split(",")]
    if len(parts) not in (1, 2):
        return None
    try:
        state = int(parts[0])
    except ValueError:
        return None
    if not 0 <= state < NUM_STATES:
        print(f"  state must be 0-{NUM_STATES - 1}")
        return None

    if len(parts) == 1 or parts[1].upper() == "X":
        arg = "X"
    else:
        try:
            arg = str(int(parts[1]))
        except ValueError:
            return None
    return f"{CMD_PREFIX} set: STATE={state},{arg}"


async def receiver(ws):
    async for msg in ws:
        print(f"\r<- {msg}\n> ", end="", flush=True)


async def sender(ws):
    loop = asyncio.get_running_loop()
    while True:
        print("> ", end="", flush=True)
        line = await loop.run_in_executor(None, sys.stdin.readline)
        if not line:
            return
        line = line.strip()
        if not line:
            continue
        if line.lower() in ("q", "quit"):
            return
        cmd = build_command(line)
        if cmd is None:
            print("  format: STATE[,VALUE]   e.g. 1,300")
            continue
        await ws.send(cmd)
        print(f"-> {cmd}")


class AuthError(Exception):
    pass


async def authenticate(ws, timeout: float = 5.0):
    await ws.send(AUTH_ID)
    loop = asyncio.get_running_loop()
    deadline = loop.time() + timeout
    while True:
        remaining = deadline - loop.time()
        if remaining <= 0:
            raise AuthError("no auth reply from server")
        msg = await asyncio.wait_for(ws.recv(), remaining)
        print(f"<- {msg}")
        if '"error"' in msg:
            raise AuthError(msg)
        if '"authenticated"' in msg and '"ok"' in msg:
            return


async def main(url: str):
    while True:
        try:
            print(f"Connecting to {url} ...")
            async with websockets.connect(url, ping_interval=20) as ws:
                await authenticate(ws)
                print(f"Authenticated as {AUTH_ID}. Type command:")
                recv_task = asyncio.create_task(receiver(ws))
                await sender(ws)
                recv_task.cancel()
                return
        except AuthError as e:
            print(f"Authentication failed: {e}")
            return
        except (OSError, asyncio.TimeoutError, websockets.ConnectionClosed) as e:
            print(f"Connection lost ({e}); retrying in 2 s")
            await asyncio.sleep(2)


if __name__ == "__main__":
    if len(sys.argv) > 2:
        print(__doc__)
        sys.exit(1)
    url = sys.argv[1] if len(sys.argv) == 2 else DEFAULT_URL
    try:
        asyncio.run(main(url))
    except KeyboardInterrupt:
        pass
