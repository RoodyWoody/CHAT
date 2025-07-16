CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    socket INET,
    username TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL,
);