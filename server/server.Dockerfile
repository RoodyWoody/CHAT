FROM boost:1.88.0
FROM python:3.10-slim

WORKDIR /usr/src/server

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

RUN g++ -std=c++17 Server_Boost.cpp -o server \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lboost_system -lpthread
    #-lboost_system -lboost_filesystem -lboost_thread -lpthread

CMD ["./server"]