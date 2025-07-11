FROM boost:1.88.0

WORKDIR /usr/src/app

COPY . .

RUN g++ -std=c++17 main.cpp -o my_app \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lboost_system -lboost_filesystem -lboost_thread

CMD ["./my_app"]