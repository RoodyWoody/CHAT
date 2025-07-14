FROM boost:1.88.0

WORKDIR /usr/src/client

COPY . .

RUN g++ -std=c++17 Client_Boost.cpp -o client \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lboost_system -lpthread
    #-lboost_system -lboost_filesystem -lboost_thread -lpthread

ENTRYPOINT ["./client"]