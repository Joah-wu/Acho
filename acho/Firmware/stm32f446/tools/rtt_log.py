#!/usr/bin/env python3
"""Read the local OpenOCD RTT channel; no serial port or extra packages needed."""
import argparse
import socket
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=9090)
    args = parser.parse_args()
    try:
        with socket.create_connection(("127.0.0.1", args.port), timeout=3) as stream:
            stream.settimeout(None)
            print("RTT connected. Ctrl+C to close the viewer.", file=sys.stderr)
            while True:
                data = stream.recv(4096)
                if not data:
                    break
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()
    except KeyboardInterrupt:
        pass
    except OSError as error:
        print(f"RTT connection failed: {error}", file=sys.stderr)
        print("Start: openocd -f DapLink.cfg -f tools/rtt.cfg", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
