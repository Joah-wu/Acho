from django.shortcuts import render, HttpResponse
from django.http import JsonResponse
from appWelcome import models
from libs.token import UserTokenGenerator
from django.contrib.auth.hashers import make_password, check_password, identify_hasher
from django.utils.crypto import constant_time_compare
from django.views.decorators.http import require_POST
from django.db import transaction


def register(request):
    if request.method == 'POST':
        user_name = request.POST.get('username')
        password = request.POST.get('password')
        res = models.User.objects.create(password=password, user_name=user_name)
        if res:
            return render(request, 'register_success.html', locals())
    else:
        return render(request, 'register.html')


def login(request):
    if request.method == 'GET':
        return render(request, 'login.html')
    else:
        return render(request, "error500page/500.html")


def userCheck(request):  # 1表示存在，0表示不存在
    if request.method == 'GET':
        user_input_name = request.GET.get('username')
        try_user = models.User.objects.filter(user_name=user_input_name).first()
        res = {"status": "NoFound"}
        if try_user:
            res['status'] = "Found"
        return JsonResponse(res)


def passwordCheck(request):
    if request.method == 'GET':
        user_input_name = request.GET.get('username')
        user_input_password = request.GET.get('password')
        try_user = models.User.objects.filter(user_name=user_input_name).first()
        res = {"status": "invalid-user"}
        if try_user:
            if verify_password(user_input_password, try_user.password):
                user_id = try_user.id
                info = {
                    'user_id': user_id,
                    'user_name': user_input_name,
                }
                token = UserTokenGenerator.dumps(info).decode('utf-8')
                try_user.is_active = True
                try_user.save()
                res["status"] = "corrected"
                res["token"] = token
            else:
                res["status"] = "uncorrected"
        return JsonResponse(res)


def hello(request):
    return render(request, 'login_expired.html')


def verify_password(password, stored):
    if not isinstance(password, str) or not isinstance(stored, str):
        return False
    try:
        identify_hasher(stored)
    except ValueError:
        return constant_time_compare(password, stored)
    return check_password(password, stored)


def api_session(user):
    token = UserTokenGenerator.dumps({'user_id': user.id, 'user_name': user.user_name}).decode('utf-8')
    user.is_active = True
    user.save(update_fields=['is_active'])
    return JsonResponse({'status': 'successful', 'token': token, 'username': user.user_name})


@require_POST
def apiLogin(request):
    username = request.POST.get('username', '').strip()
    password = request.POST.get('password', '')
    if not username or len(username) > 255 or not password or len(password) > 128:
        return JsonResponse({'status': 'failed', 'message': '请输入有效的用户名和密码'}, status=400)
    user = models.User.objects.filter(user_name=username).first()
    if user is None or not verify_password(password, user.password):
        return JsonResponse({'status': 'failed', 'message': '用户名或密码不正确'}, status=401)
    return api_session(user)


@require_POST
def apiRegister(request):
    username = request.POST.get('username', '').strip()
    password = request.POST.get('password', '')
    if not username or len(username) > 255 or not 8 <= len(password) <= 128:
        return JsonResponse({'status': 'failed', 'message': '用户名不能为空，密码需要 8～128 位'}, status=400)
    with transaction.atomic():
        if models.User.objects.filter(user_name=username).exists():
            return JsonResponse({'status': 'failed', 'message': '用户名已存在，请直接登录'}, status=409)
        user = models.User.objects.create(user_name=username, password=make_password(password))
    return api_session(user)
