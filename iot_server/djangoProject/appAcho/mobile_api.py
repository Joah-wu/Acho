"""User-facing Android file management; all operations enforce device ownership."""
import os
from django.http import JsonResponse, FileResponse, HttpResponseNotAllowed
from libs.token import user_token_checkAjax
from .models import UploadedFile, UploadFileForm

MAX_FILE = 32 * 1024 * 1024


def failure(message, status=400):
    return JsonResponse({'status': 'failed', 'message': message}, status=status)


def owned_device(user, value):
    try:
        return user.devices.filter(pk=int(value)).first()
    except (ValueError, TypeError, OverflowError):
        return None


def owned_file(user, device_id, file_id):
    device = owned_device(user, device_id)
    if device is None:
        return None
    try:
        return UploadedFile.objects.filter(device=device, pk=int(file_id)).first()
    except (ValueError, TypeError, OverflowError):
        return None


@user_token_checkAjax
def files(request, data, user):
    if request.method == 'GET':
        device = owned_device(user, request.GET.get('dev_id'))
        if device is None:
            return failure('设备不存在或尚未绑定', 403)
        try:
            kind = int(request.GET.get('file_type'))
        except (TypeError, ValueError):
            return failure('文件类型无效')
        if kind not in (1, 2):
            return failure('文件类型无效')
        rows = UploadedFile.objects.filter(device=device, type=kind).order_by('-uploaded_at', '-id')
        output = []
        for item in rows:
            try:
                size = item.file.size
            except OSError:
                size = 0
            output.append({'id': item.id, 'name': os.path.basename(item.file.name.replace('\\', '/')),
                           'description': item.description, 'type': item.type, 'size': size,
                           'uploaded_at': item.uploaded_at.isoformat()})
        data['data'] = output
        return JsonResponse(data)
    if request.method != 'POST':
        return HttpResponseNotAllowed(['GET', 'POST'])
    form = UploadFileForm(request.POST, request.FILES)
    if not form.is_valid():
        return failure('请选择文件，说明不超过 200 字')
    device = owned_device(user, form.cleaned_data['device_id'])
    if device is None:
        return failure('设备不存在或尚未绑定', 403)
    upload = form.cleaned_data['file']
    kind = form.cleaned_data['type']
    if kind not in (1, 2) or not 0 < upload.size <= MAX_FILE:
        return failure('请选择不超过 32 MiB 的非空图片或 WAV 文件')
    if kind == 2:
        header = upload.read(12)
        upload.seek(0)
        if not upload.name.lower().endswith('.wav') or header[:4] != b'RIFF' or header[8:12] != b'WAVE':
            return failure('音频需要使用 WAV 文件')
    else:
        from PIL import Image, UnidentifiedImageError
        try:
            with Image.open(upload) as image:
                image.verify()
            upload.seek(0)
        except (OSError, ValueError, UnidentifiedImageError):
            return failure('图片无法解码，请使用有效的 JPEG、PNG、BMP 或 WebP')
    item = UploadedFile.objects.create(device=device, type=kind, file=upload,
                                      description=form.cleaned_data['description'])
    data['file_id'] = item.pk
    return JsonResponse(data)


@user_token_checkAjax
def content(request, data, user):
    if request.method != 'GET':
        return HttpResponseNotAllowed(['GET'])
    item = owned_file(user, request.GET.get('dev_id'), request.GET.get('file_id'))
    if item is None:
        return failure('文件不存在或无权访问', 404)
    try:
        if item.file.size > MAX_FILE:
            return failure('文件超过 32 MiB')
        stream = item.file.open('rb')
    except OSError:
        return failure('文件已丢失，请重新上传', 404)
    return FileResponse(stream, content_type='application/octet-stream', as_attachment=True,
                        filename=os.path.basename(item.file.name.replace('\\', '/')))


@user_token_checkAjax
def delete(request, data, user):
    if request.method != 'POST':
        return HttpResponseNotAllowed(['POST'])
    item = owned_file(user, request.POST.get('dev_id'), request.POST.get('file_id'))
    if item is None:
        return failure('文件不存在或无权访问', 404)
    item.file.delete(save=False)
    item.delete()
    return JsonResponse(data)
