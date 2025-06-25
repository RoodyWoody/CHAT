# 🚀 C++ Chat Application

Кроссплатформенный чат на основе клиент-серверной архитектуры с графическим интерфейсом для клиентов.  

===

# Для Linux:
## Server.cpp
### Сборка
g++ Server.cpp -o Server -lpthread -std=c++17

### Запуск
./Server

---

## Client_app.cpp (клиент на GTKMM)
### Сборка
g++ client_app.cpp -o chatclient $(pkg-config --cflags --libs gtkmm-3.0) -lgobject-2.0 -lglib-2.0

### Запуск
./chatclient

---

## Клиент Linux для консоли в разработке

===

# Windows
## Server.cpp
### Сборка
g++ Server.cpp -o Server -lpthread -std=c++17 -lws2_32

### Запуск
./Server

---

## Client_app.cpp (клиент на GTKMM)
### Сборка
x86_64-w64-mingw32-g++ client_app.cpp -o chatclient $(pkg-config --cflags --libs gtkmm-3.0) -lgobject-2.0 -lglib-2.0

### Запуск
./chatclient.exe

или 

Запускаем MSYS2 MinGW x64
### Сборка
g++ client.cpp -o chatclient -mwindows pkg-config --cflags --libs gtkmm-3.0 -lgobject-2.0 -lglib-2.0

### Запуск
./chatclient

---

## Сlient_console_win.cpp
### Сборка
g++ client_console_win.cpp -o client_console -lpthread -std=c++17 -lws2_32

### Запуск
./client_console
