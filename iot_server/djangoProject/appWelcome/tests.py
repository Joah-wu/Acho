
from django.test import TestCase

from appWelcome.models import User, UserAuthority


class RegistrationTests(TestCase):
    def test_default_user_authority_is_initialized(self):
        authority_id = User._meta.get_field('authority').get_default()
        self.assertTrue(UserAuthority.objects.filter(pk=authority_id).exists())

    def test_register_saves_user_with_default_authority(self):
        response = self.client.post('/welcome/register/', {
            'username': 'registration-test',
            'password': 'registration-test-password',
        })

        self.assertEqual(response.status_code, 200)
        self.assertTemplateUsed(response, 'register_success.html')
        user = User.objects.get(user_name='registration-test')
        self.assertEqual(user.authority_id, User._meta.get_field('authority').get_default())
        self.assertEqual(user.authority.authority_level, 'user')
