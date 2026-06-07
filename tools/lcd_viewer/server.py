"""CLI entry point for the LCD framebuffer web viewer.

Usage:
    python tools/lcd_viewer/server.py [--port 8765] [--period 50] [--host 127.0.0.1]

Then open http://127.0.0.1:8765/ in a browser.
"""
from __future__ import annotations

import argparse
import asyncio
import logging
import signal
import sys

from tools.lcd_viewer import config
from tools.lcd_viewer.dumper import Dumper
from tools.lcd_viewer.ws_server import WSServer


def _parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        prog="lcd_viewer",
        description="Stream the LCD framebuffer to a local browser.",
    )
    p.add_argument("--host", default=config.HOST, help="bind host (default 127.0.0.1)")
    p.add_argument("--port", type=int, default=config.PORT, help=f"port (default {config.PORT})")
    p.add_argument(
        "--period",
        type=int,
        default=config.DEFAULT_PERIOD_MS,
        help=f"sampling period in ms (default {config.DEFAULT_PERIOD_MS})",
    )
    p.add_argument("-v", "--verbose", action="store_true", help="enable debug logging")
    return p.parse_args()


async def _main(args: argparse.Namespace) -> int:
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(name)s %(levelname)s %(message)s",
        stream=sys.stderr,
    )
    dumper = Dumper()
    server = WSServer(dumper)
    await server.start(host=args.host, port=args.port)
    print(f"[lcd_viewer] Open http://{args.host}:{args.port}/ in a browser", file=sys.stderr)
    print(f"[lcd_viewer] Default period: {args.period} ms", file=sys.stderr)
    # Wait until SIGINT / SIGTERM.
    stop = asyncio.Event()
    loop = asyncio.get_running_loop()
    for sig in (signal.SIGINT, signal.SIGTERM):
        try:
            loop.add_signal_handler(sig, stop.set)
        except (NotImplementedError, RuntimeError):
            pass  # Windows: signal handlers may not be supported in all envs
    try:
        await stop.wait()
    finally:
        print("[lcd_viewer] shutting down...", file=sys.stderr)
        await server.stop()
        await dumper.stop()
    return 0


def main() -> int:
    args = _parse_args()
    return asyncio.run(_main(args))


if __name__ == "__main__":
    raise SystemExit(main())
