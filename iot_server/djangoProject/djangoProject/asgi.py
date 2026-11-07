"""
ASGI config for djangoProject project.

It exposes the ASGI callable as a module-level variable named ``application``.

For more information on this file, see
https://docs.djangoproject.com/en/3.2/howto/deployment/asgi/
"""

import os

from django.core.asgi import get_asgi_application


os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'djangoProject.settings')

django_asgi_application = get_asgi_application()

# Import auth middleware and consumers after initializing the model registry.
from channels.auth import AuthMiddlewareStack
from channels.routing import ProtocolTypeRouter, URLRouter
import appMyBroker.routing

application = ProtocolTypeRouter({
    # http请求使用这个
    "http": django_asgi_application,

    # websocket请求使用这个
    "websocket": AuthMiddlewareStack(
        URLRouter(
            appMyBroker.routing.websocket_urlpatterns
        )
    ),
})
