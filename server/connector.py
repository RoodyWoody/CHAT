import psycopg2
import os
from psycopg2 import sql

DB_HOST = os.getenv("DB_HOST", "postgres")
DB_PORT = os.getenv("DB_PORT", "5432")
DB_NAME = os.getenv("DB_NAME", "postgres_db")
DB_USER = os.getenv("DB_USER", "admin")
DB_PASSWORD = os.getenv("DB_PASSWORD", "admin")


def connect_to_db():
    try:
        conn = psycopg2.connect(
            host=DB_HOST,
            port=DB_PORT,
            dbname=DB_NAME,
            user=DB_USER,
            password=DB_PASSWORD,
        )
        print("Подключение к PostgreSQL установлено")
        return conn
    except Exception as e:
        print(f"Ошибка подключения к PostgreSQL: {e}")
        return None


'''def create_table(conn):
    try:
        with conn.cursor() as cur:
            cur.execute(
                """
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    name VARCHAR(100) NOT NULL,
                    email VARCHAR(100) UNIQUE NOT NULL
                )
            """
            )
            conn.commit()
            print("Таблица 'users' создана или уже существует")
    except Exception as e:
        print(f"Ошибка при создании таблицы: {e}")
'''


def insert_user(conn, socket, username, password):
    try:
        with conn.cursor() as cur:
            cur.execute(
                sql.SQL(
                    "INSERT INTO users (socket, username, password) VALUES (%s, %s)"
                ),
                (socket, username, password),
            )
            conn.commit()
            print(f"Пользователь {username} добавлен")
    except Exception as e:
        print(f"Ошибка при вставке данных: {e}")


def select_users(conn):
    try:
        with conn.cursor() as cur:
            cur.execute("SELECT * FROM users")
            rows = cur.fetchall()
            print("Список пользователей:")
            for row in rows:
                print(row)
    except Exception as e:
        print(f"Ошибка при выборке данных: {e}")


if __name__ == "__main__":
    connection = connect_to_db()
    if connection:
        # create_table(connection)

        insert_user(connection, "Alice", "alice@example.com")
        insert_user(connection, "Bob", "bob@example.com")

        select_users(connection)

        connection.close()
        print("Соединение с PostgreSQL закрыто")
