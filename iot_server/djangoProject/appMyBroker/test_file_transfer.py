import io
import json
import tempfile
import zlib
from unittest.mock import patch

from asgiref.sync import async_to_sync
from channels.routing import URLRouter
from channels.testing import WebsocketCommunicator
from django.core.files.uploadedfile import SimpleUploadedFile
from django.test import TransactionTestCase, override_settings
from itsdangerous import TimedJSONWebSignatureSerializer
from PIL import Image

from appWelcome.models import Device, DeviceType
from appAcho.models import UploadedFile
from libs.token import (DeviceTokenGenerator, device_token_salt, BindTokenGenerator,
                        checkBindToken, UserTokenGenerator)
from .routing import websocket_urlpatterns


class FileTransferTests(TransactionTestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        settings = override_settings(MEDIA_ROOT=temporary.name)
        settings.enable()
        self.addCleanup(settings.disable)
        kind = DeviceType.objects.create(type='Acho')
        self.device = Device.objects.create(mac='00:11:22:33:44:55', ip='192.0.2.1', type=kind)
        self.other = Device.objects.create(mac='00:11:22:33:44:56', ip='192.0.2.2', type=kind)
        self.token = DeviceTokenGenerator.dumps({'device_id': self.device.pk}).decode()
        self.data = bytes(range(256)) * 19 + b'\x00\xa5\x5a\n'
        self.audio = self.upload('sample.wav', self.data, 2)

    def upload(self, name, data, kind, device=None, description='test'):
        return UploadedFile.objects.create(device=device or self.device, type=kind,
                                           description=description,
                                           file=SimpleUploadedFile(name, data))

    async def socket(self):
        socket = WebsocketCommunicator(URLRouter(websocket_urlpatterns), '/broker/ws/files/')
        connected, _ = await socket.connect()
        self.assertTrue(connected)
        return socket

    async def download(self, socket, **request):
        await socket.send_json_to(dict(token=self.token, **request))
        begin = await socket.receive_json_from(timeout=5)
        self.assertEqual(begin['event'], 'begin')
        self.assertEqual(begin['chunk_size'], 1024)
        data = bytearray()
        while True:
            await socket.send_json_to({'op': 'next', 'offset': len(data)})
            response = await socket.receive_from(timeout=5)
            if isinstance(response, str):
                end = json.loads(response)
                self.assertEqual(end['event'], 'end')
                self.assertEqual(end['size'], len(data))
                self.assertEqual(end['crc32'], zlib.crc32(data) & 0xffffffff)
                self.assertEqual(begin['size'], len(data))
                return bytes(data)
            self.assertGreater(len(response), 0)
            self.assertLessEqual(len(response), 1024)
            data.extend(response)

    @async_to_sync
    async def test_binary_chunks_and_crc(self):
        socket = await self.socket()
        self.assertEqual(await self.download(socket, op='get_audio_file', file_id=self.audio.pk), self.data)
        # Same connection supports another transfer after end.
        self.assertEqual(await self.download(socket, op='get_audio_file', file_id=self.audio.pk), self.data)
        await socket.disconnect()

    def test_empty_file(self):
        empty = self.upload('empty.wav', b'', 2)
        async def run():
            socket = await self.socket()
            self.assertEqual(await self.download(socket, op='get_audio_file', file_id=empty.pk), b'')
            await socket.disconnect()
        async_to_sync(run)()

    def test_device_ownership_and_file_type(self):
        foreign = self.upload('private.wav', b'private', 2, self.other)
        async def run():
            socket = await self.socket()
            for request in ({'op': 'get_audio_file', 'file_id': foreign.pk},
                            {'op': 'get_dither_bin', 'file_id': self.audio.pk},
                            {'op': 'get_audio_file', 'file_id': '../private.wav'}):
                await socket.send_json_to(dict(token=self.token, **request))
                self.assertEqual((await socket.receive_json_from())['event'], 'error')
            await socket.disconnect()
        async_to_sync(run)()

    @async_to_sync
    async def test_invalid_and_expired_token(self):
        socket = await self.socket()
        expired = TimedJSONWebSignatureSerializer(device_token_salt, expires_in=-1).dumps(
            {'device_id': self.device.pk}).decode()
        for token in ('', 'invalid', expired):
            await socket.send_json_to({'op': 'get_file_list', 'file_type': 1, 'token': token})
            self.assertEqual((await socket.receive_json_from())['event'], 'error')
        await socket.disconnect()

    @async_to_sync
    async def test_malformed_input_offset_and_cancel(self):
        socket = await self.socket()
        for invalid in ('{', '[]', 'x' * 4097):
            await socket.send_to(text_data=invalid)
            self.assertEqual((await socket.receive_json_from())['event'], 'error')
        await socket.send_to(bytes_data=b'not JSON')
        self.assertEqual((await socket.receive_json_from())['event'], 'error')
        await socket.send_json_to({'op': 'get_audio_file', 'file_id': self.audio.pk, 'token': self.token})
        self.assertEqual((await socket.receive_json_from())['event'], 'begin')
        await socket.send_json_to({'op': 'next', 'offset': 1})
        self.assertEqual((await socket.receive_json_from())['event'], 'error')
        await socket.send_json_to({'op': 'next', 'offset': 0})
        self.assertEqual((await socket.receive_json_from())['event'], 'error')
        await socket.send_json_to({'op': 'get_audio_file', 'file_id': self.audio.pk, 'token': self.token})
        await socket.receive_json_from()
        await socket.send_json_to({'op': 'cancel'})
        self.assertEqual((await socket.receive_json_from())['event'], 'cancelled')
        await socket.disconnect()

    def test_file_list_escaping_and_isolation(self):
        self.upload('PHOTO.png', b'image', 1, description='a\tb\nc\r')
        self.upload('hidden.png', b'image', 1, self.other)
        async def run():
            socket = await self.socket()
            data = (await self.download(socket, op='get_file_list', file_type=1)).decode()
            self.assertIn('photo.png\ta b c \n', data)
            self.assertNotIn('hidden', data)
            self.assertEqual(len(data.splitlines()), 1)
            await socket.disconnect()
        async_to_sync(run)()

    def test_image_and_bind_qr(self):
        image = io.BytesIO()
        Image.new('RGB', (160, 160), 'white').save(image, 'PNG')
        picture = self.upload('white.png', image.getvalue(), 1)
        async def run():
            socket = await self.socket()
            for request in ({'op': 'get_dither_bin', 'file_id': picture.pk, 'size': [128, 128]},
                            {'op': 'get_bind_QR', 'size': [128, 128]}):
                data = await self.download(socket, **request)
                header = int.from_bytes(data[:4], 'little')
                self.assertEqual(header & 31, 7)
                width, height = (header >> 10) & 2047, (header >> 21) & 2047
                self.assertLessEqual(width, 128)
                self.assertLessEqual(height, 128)
                self.assertEqual(len(data), 12 + ((width + 7) // 8) * height)
            for size in ('[128,128]', [0, 128], [True, 128], [1024, 128]):
                await socket.send_json_to({'op': 'get_bind_QR', 'size': size, 'token': self.token})
                self.assertEqual((await socket.receive_json_from())['event'], 'error')
            await socket.disconnect()
        async_to_sync(run)()

    def test_nfc_binding_token_uses_authenticated_device(self):
        async def run():
            socket = await self.socket()
            data = await self.download(socket, op='get_bind_token', device_id=self.other.pk)
            await socket.disconnect()
            return data
        token = async_to_sync(run)().decode('utf-8')
        self.assertLessEqual(len(token.encode('utf-8')), 384)
        self.assertRegex(token, r'^[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+$')
        status, device = checkBindToken(token)
        self.assertEqual(status, 0)
        self.assertEqual(device.pk, self.device.pk)
        header = BindTokenGenerator.loads(token, return_header=True)[1]
        self.assertEqual(header['exp'] - header['iat'], 60)

    @async_to_sync
    async def test_nfc_binding_token_requires_device_authentication(self):
        socket = await self.socket()
        user_token = UserTokenGenerator.dumps({'user_id': 1}).decode()
        for token in ('', 'invalid', user_token):
            await socket.send_json_to({'op': 'get_bind_token', 'token': token})
            self.assertEqual((await socket.receive_json_from())['event'], 'error')
        await socket.disconnect()

    def test_disconnect_releases_stream(self):
        stream = io.BytesIO(b'x' * 2048)
        async def run():
            socket = await self.socket()
            await socket.send_json_to({'op': 'get_audio_file', 'file_id': 1, 'token': self.token})
            await socket.receive_json_from()
            await socket.disconnect()
        with patch('appMyBroker.file_transfer.open_source', return_value=(stream, 2048)):
            async_to_sync(run)()
        self.assertTrue(stream.closed)
