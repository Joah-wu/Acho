"""Device-only, pull-based file transport. One bounded chunk per `next`."""
import io
import json
import os
import zlib

from channels.generic.websocket import WebsocketConsumer
from cv2 import error as ImageError
from qrcode.exceptions import DataOverflowError
from appAcho.models import UploadedFile
from libs.token import checkDeviceToken, BindTokenGenerator
from libs import dither

CHUNK_SIZE = 1024
MAX_SIZE = 32 * 1024 * 1024


def screen_size(value):
    if (not isinstance(value, list) or len(value) != 2 or
            any(type(n) is not int or not 1 <= n <= 512 for n in value)):
        raise ValueError('invalid_screen_size')
    return value


def open_source(device, request):
    """Return an owned binary stream and its exact length; never accept paths."""
    op = request.get('op')
    if op == 'get_file_list':
        kind = request.get('file_type')
        if type(kind) is not int or kind not in (0, 1, 2):
            raise ValueError('invalid_file_type')
        def clean(value):
            return str(value).replace('\t', ' ').replace('\r', ' ').replace('\n', ' ')
        rows = UploadedFile.objects.filter(device=device, type=kind).order_by('id')
        data = bytearray()
        for item in rows.iterator():
            name = os.path.basename(item.file.name.replace('\\', '/')).lower()
            data.extend(f'{item.id}\t{clean(name)}\t{clean(item.description)}\n'.encode('utf-8'))
            if len(data) > 128 * 1024:
                raise ValueError('file_list_too_large')
        return io.BytesIO(data), len(data)
    if op in ('get_bind_QR', 'get_bind_token'):
        token = BindTokenGenerator.dumps({'id': device.id, 'ip': device.ip}).decode('utf-8')
        if op == 'get_bind_token':
            data = token.encode('utf-8')
            return io.BytesIO(data), len(data)
        data = dither.getQRCodeData(token, screen_size(request.get('size', [128, 128])))
        return io.BytesIO(data), len(data)
    if op not in ('get_dither_bin', 'get_audio_file'):
        raise ValueError('unknown_operation')
    file_id = request.get('file_id')
    if type(file_id) is not int or file_id <= 0:
        raise ValueError('invalid_file_id')
    kind = 1 if op == 'get_dither_bin' else 2
    item = UploadedFile.objects.filter(pk=file_id, device=device, type=kind).first()
    if item is None:
        raise ValueError('file_not_found')
    if kind == 1:
        data = dither.getDitherImgBinData(item.file.path, screen_size(request.get('size', [128, 128])))
        return io.BytesIO(data), len(data)
    size = item.file.size
    if size > MAX_SIZE:
        raise ValueError('file_too_large')
    return item.file.open('rb'), size


class FileTransferConsumer(WebsocketConsumer):
    def connect(self):
        self.source = None
        self.accept()

    def release(self):
        if self.source is not None:
            self.source.close()
            self.source = None

    def reply(self, **data):
        self.send(text_data=json.dumps(data, separators=(',', ':')))

    def receive(self, text_data=None, bytes_data=None):
        try:
            if text_data is None or len(text_data) > 4096:
                raise ValueError('invalid_request')
            request = json.loads(text_data)
            if not isinstance(request, dict):
                raise ValueError('invalid_request')
            op = request.get('op')
            if op == 'cancel':
                self.release()
                self.reply(event='cancelled')
            elif op == 'next':
                if self.source is None:
                    raise ValueError('no_transfer')
                if type(request.get('offset')) is not int or request['offset'] != self.offset:
                    raise ValueError('invalid_offset')
                if self.offset == self.size:
                    self.release()
                    self.reply(event='end', size=self.size, crc32=self.crc)
                    return
                data = self.source.read(min(CHUNK_SIZE, self.size - self.offset))
                if not data:
                    raise ValueError('unexpected_eof')
                self.offset += len(data)
                self.crc = zlib.crc32(data, self.crc) & 0xffffffff
                self.send(bytes_data=data)
            else:
                if self.source is not None:
                    raise ValueError('transfer_busy')
                token = request.get('token')
                if not isinstance(token, str) or not token:
                    raise ValueError('invalid_token')
                result, device = checkDeviceToken(token)
                if result != 0:
                    raise ValueError('expired_token' if result == 1 else 'invalid_token')
                self.source, self.size = open_source(device, request)
                if not 0 <= self.size <= MAX_SIZE:
                    raise ValueError('file_too_large')
                self.offset = self.crc = 0
                self.reply(event='begin', size=self.size, chunk_size=CHUNK_SIZE)
        except (ValueError, TypeError, KeyError, OSError, OverflowError, ImageError, DataOverflowError, AttributeError):
            self.release()
            self.reply(event='error', error='request_failed')

    def disconnect(self, close_code):
        self.release()
