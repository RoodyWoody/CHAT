CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    socket INET,
    username TEXT NOT NULL UNIQUE,
    user_password TEXT NOT NULL
);