FROM boost:1.88.0 AS builder

WORKDIR /usr/src/server

COPY Server_Boost.cpp .

RUN g++ -std=c++17 Server_Boost.cpp -o /server \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lboost_system -lpthread

FROM python:3.10-slim

RUN apt-get update && \
    apt-get install -y libpq-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY connector.py .

COPY --from=builder /server /app/server

RUN chmod +x /app/server

CMD ["./server"]