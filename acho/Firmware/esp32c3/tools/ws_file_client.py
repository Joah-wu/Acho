#!/usr/bin/env python3
"""Exercise the same registration + file WebSocket flow as the ESP32 bridge."""
import argparse
import asyncio
import json
from pathlib import Path
import zlib

import requests
import websockets


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server', required=True, help='http[s]://host:port')
    parser.add_argument('--mac', required=True, help='MAC of a development device')
    parser.add_argument('--ip', default='127.0.0.1')
    parser.add_argument('--op', choices=['get_file_list', 'get_dither_bin', 'get_audio_file', 'get_bind_QR'],
                        default='get_file_list')
    parser.add_argument('--file-type', type=int, choices=[0, 1, 2], default=1)
    parser.add_argument('--file-id', type=int)
    parser.add_argument('--size', type=int, nargs=2, default=[128, 128], metavar=('HEIGHT', 'WIDTH'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.op in ('get_dither_bin', 'get_audio_file') and not args.file_id:
        parser.error('--file-id is required for image/audio downloads')
    return args


async def download(args, token):
    server = args.server.rstrip('/')
    uri = ('wss://' + server[8:] if server.startswith('https://') else 'ws://' + server[7:])
    request = dict(op=args.op, token=token, file_type=args.file_type, file_id=args.file_id, size=args.size)
    temporary = args.output.with_name(args.output.name + '.part')
    try:
        async with websockets.connect(uri + '/broker/ws/files/', max_size=4096, open_timeout=15) as socket:
            await socket.send(json.dumps(request))
            begin = json.loads(await asyncio.wait_for(socket.recv(), 15))
            if begin.get('event') != 'begin':
                raise RuntimeError(begin)
            offset = crc = 0
            with temporary.open('wb') as output:
                while True:
                    await socket.send(json.dumps(dict(op='next', offset=offset)))
                    data = await asyncio.wait_for(socket.recv(), 15)
                    if isinstance(data, str):
                        end = json.loads(data)
                        if (end.get('event') != 'end' or end.get('size') != offset or
                                begin['size'] != offset or end.get('crc32') != crc):
                            raise RuntimeError(f'Invalid completion: {end}')
                        break
                    if not data or len(data) > begin['chunk_size'] or offset + len(data) > begin['size']:
                        raise RuntimeError('Invalid data chunk')
                    output.write(data)
                    offset += len(data)
                    crc = zlib.crc32(data, crc) & 0xffffffff
            temporary.replace(args.output)
            print(f'Saved {offset} bytes, CRC32={crc:08x}: {args.output}')
    finally:
        temporary.unlink(missing_ok=True)


def main():
    args = arguments()
    response = requests.get(args.server.rstrip('/') + '/home/device/init/',
                            params=dict(mac=args.mac, ip=args.ip, type='Acho'), timeout=15)
    response.raise_for_status()
    registration = response.json()
    if registration.get('status') != 'successful':
        raise RuntimeError('Device registration failed; ensure DeviceType "Acho" exists')
    asyncio.run(download(args, registration['token']))


if __name__ == '__main__':
    main()
