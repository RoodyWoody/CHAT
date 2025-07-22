FROM boost:1.88.0 AS builder

WORKDIR /usr/src/server

COPY Server_Boost.cpp .
COPY json.hpp .
COPY query_test.cpp .

RUN g++ -std=c++17 Server_Boost.cpp -o /server \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lboost_system -lpthread

RUN g++ -std=c++17 query_test.cpp -o /query_test \
    -I/usr/local/include \    
    -I/usr/src/server \        
    -lpthread

FROM python:3.10-slim

RUN apt-get update && \
    apt-get install -y libpq-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY db_query.py .

COPY --from=builder /server /app/server
COPY --from=builder /query_test /app/query_test

RUN chmod +x /app/server

CMD ["./server"]