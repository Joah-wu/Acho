from django.urls import re_path

from . import consumer
from .file_transfer import FileTransferConsumer

websocket_urlpatterns = [
    re_path(r'^broker/ws/files/$', FileTransferConsumer.as_asgi()),
    re_path(r'broker/ws/(?P<operate>.*?)/(?P<topic>.*?)/(?P<receive_mode>.*?)/(?P<token>.*?)$',
            consumer.BaseConsumer.as_asgi()),
]
