import io
import tempfile
from django.test import TestCase, override_settings
from django.core.files.uploadedfile import SimpleUploadedFile
from django.contrib.auth.hashers import check_password
from PIL import Image
from itsdangerous import TimedJSONWebSignatureSerializer
from appWelcome.models import User, Device, DeviceType
from libs.token import UserTokenGenerator, BindTokenGenerator, bind_token_salt
from .models import UploadedFile


class MobileApiTests(TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        settings = override_settings(MEDIA_ROOT=temporary.name)
        settings.enable()
        self.addCleanup(settings.disable)
        self.user = User.objects.create(user_name='mobile', password='legacy-pass')
        self.token = UserTokenGenerator.dumps({'user_id': self.user.pk}).decode()
        kind = DeviceType.objects.create(type='Acho')
        self.device = Device.objects.create(mac='02:00:00:00:00:01', ip='192.0.2.1', type=kind)
        self.foreign = Device.objects.create(mac='02:00:00:00:00:02', ip='192.0.2.2', type=kind)
        self.user.devices.add(self.device)

    def image(self):
        image = io.BytesIO()
        Image.new('RGB', (64, 32), 'white').save(image, 'PNG')
        return SimpleUploadedFile('mobile.png', image.getvalue(), 'image/png')

    def upload(self, device=None, **changes):
        values = {'tok': self.token, 'device_id': (device or self.device).pk,
                  'type': 1, 'description': '来自安卓', 'file': self.image()}
        values.update(changes)
        return self.client.post('/acho/api/files/', values)

    def test_register_login_and_legacy_login(self):
        response = self.client.post('/welcome/api/register/', {'username': 'new-user', 'password': 'strong-pass'})
        self.assertEqual(response.status_code, 200)
        self.assertTrue(response.json()['token'])
        user = User.objects.get(user_name='new-user')
        self.assertTrue(check_password('strong-pass', user.password))
        self.assertEqual(self.client.post('/welcome/api/register/', {'username': 'new-user', 'password': 'strong-pass'}).status_code, 409)
        self.assertEqual(self.client.post('/welcome/api/login/', {'username': 'new-user', 'password': 'strong-pass'}).json()['status'], 'successful')
        self.assertEqual(self.client.post('/welcome/api/login/', {'username': 'mobile', 'password': 'legacy-pass'}).json()['status'], 'successful')
        self.assertEqual(self.client.get('/welcome/password/check/', {'username': 'new-user', 'password': 'strong-pass'}).json()['status'], 'corrected')
        self.assertEqual(self.client.post('/welcome/api/login/', {'username': 'mobile', 'password': 'wrong'}).status_code, 401)
        self.assertEqual(self.client.post('/welcome/api/register/', {'username': '', 'password': 'short'}).status_code, 400)
        self.assertEqual(self.client.get('/welcome/api/login/').status_code, 405)

    def test_binding_raw_qr_token_and_expiration(self):
        token = BindTokenGenerator.dumps({'id': self.foreign.pk, 'ip': self.foreign.ip}).decode()
        response = self.client.get('/home/user/bind/', {'tok': self.token, 'bind_token': token})
        self.assertEqual(response.json()['status'], 'successful')
        self.assertTrue(self.user.devices.filter(pk=self.foreign.pk).exists())
        self.assertEqual(self.client.get('/home/user/bind/', {'tok': self.token, 'bind_token': token}).json()['status'], 'existed')
        expired = TimedJSONWebSignatureSerializer(bind_token_salt, expires_in=-1).dumps({'id': self.device.pk}).decode()
        self.assertEqual(self.client.get('/home/user/bind/', {'tok': self.token, 'bind_token': expired}).json()['status'], 'QR-expired')
        self.assertEqual(self.client.get('/home/user/bind/', {'tok': self.token}).json()['status'], 'QR-expired')

    def test_upload_list_download_delete(self):
        response = self.upload()
        self.assertEqual(response.json()['status'], 'successful')
        item = UploadedFile.objects.get(pk=response.json()['file_id'])
        self.assertNotIn('\\', item.file.name)
        params = {'tok': self.token, 'dev_id': self.device.pk, 'file_type': 1}
        listing = self.client.get('/acho/api/files/', params).json()
        self.assertEqual(len(listing['data']), 1)
        self.assertEqual(listing['data'][0]['name'], 'mobile.png')
        self.assertTrue(listing['token'])
        params['file_id'] = item.pk
        downloaded = self.client.get('/acho/api/file/', params)
        self.assertEqual(downloaded['Content-Type'], 'application/octet-stream')
        self.assertTrue(b''.join(downloaded.streaming_content).startswith(b'\x89PNG'))
        downloaded.close()
        deleted = self.client.post('/acho/api/file/delete/', params)
        self.assertEqual(deleted.json()['status'], 'successful')
        self.assertFalse(UploadedFile.objects.filter(pk=item.pk).exists())

    def test_device_and_file_ownership(self):
        self.assertEqual(self.upload(self.foreign).status_code, 403)
        file = UploadedFile.objects.create(device=self.foreign, type=1, file=self.image(), description='private')
        values = {'tok': self.token, 'dev_id': self.foreign.pk, 'file_type': 1, 'file_id': file.pk}
        self.assertEqual(self.client.get('/acho/api/files/', values).status_code, 403)
        self.assertEqual(self.client.get('/acho/api/file/', values).status_code, 404)
        self.assertEqual(self.client.post('/acho/api/file/delete/', values).status_code, 404)
        values['dev_id'] = self.device.pk
        self.assertEqual(self.client.get('/acho/api/file/', values).status_code, 404)
        self.assertTrue(UploadedFile.objects.filter(pk=file.pk).exists())

    def test_bad_inputs_and_missing_token(self):
        self.assertEqual(self.client.get('/acho/api/files/').json()['token'], 'error')
        for device_id in ('abc', '-1', str(2**100)):
            response = self.client.get('/acho/api/files/', {'tok': self.token, 'dev_id': device_id, 'file_type': 1})
            self.assertEqual(response.status_code, 403)
        self.assertEqual(self.upload(type=2, file=SimpleUploadedFile('test.wav', b'not a wave')).status_code, 400)
        self.assertEqual(self.upload(file=SimpleUploadedFile('image.png', b'not an image')).status_code, 400)
        self.assertEqual(self.upload(description='x'*201).status_code, 400)

    def test_valid_wav_and_invalid_file_type(self):
        import wave
        wav = io.BytesIO()
        with wave.open(wav, 'wb') as output:
            output.setnchannels(1)
            output.setsampwidth(2)
            output.setframerate(16000)
            output.writeframes(b'\x00\x00'*100)
        response = self.upload(type=2, file=SimpleUploadedFile('recording.wav', wav.getvalue(), 'audio/wav'))
        self.assertEqual(response.json()['status'], 'successful')
        self.assertEqual(self.upload(type=0).status_code, 400)


class ImageCompatibilityTests(TestCase):
    def test_non_square_screen_preserves_bounds(self):
        import numpy as np
        from libs.dither import reshapeImgForScreen
        image = np.zeros((350, 500, 3), dtype=np.uint8)
        resized = reshapeImgForScreen(image, [240, 400])
        self.assertLessEqual(resized.shape[0], 240)
        self.assertLessEqual(resized.shape[1], 400)

    def test_grayscale_phone_image_can_be_rendered(self):
        from libs.dither import getDitherImgBinData
        with tempfile.NamedTemporaryFile(suffix='.png') as file:
            Image.new('L', (200, 80), 128).save(file.name, 'PNG')
            data = getDitherImgBinData(file.name, [128, 128])
        header = int.from_bytes(data[:4], 'little')
        self.assertEqual(header & 31, 7)
        self.assertLessEqual((header >> 10) & 2047, 128)
        self.assertLessEqual((header >> 21) & 2047, 128)
