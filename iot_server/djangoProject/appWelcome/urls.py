from django.urls import path
from appWelcome import views

urlpatterns = [
    path('api/login/', views.apiLogin),
    path('api/register/', views.apiRegister),
    path('register/', views.register),
    path('login/', views.login),
    path('user/check/', views.userCheck),
    path('password/check/', views.passwordCheck),
    path('hello/', views.hello)
]
