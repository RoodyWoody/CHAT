FROM boost:1.88.0

WORKDIR /usr/src/server

COPY . .

RUN g++ -std=c++17 Server_Boost.cpp -o server \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lboost_system -lpthread
    #-lboost_system -lboost_filesystem -lboost_thread -lpthread

RUN apt-get install libpq-dev psycopg2

CMD ["./server"]