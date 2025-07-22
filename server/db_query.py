import psycopg2
import sys
import json
from psycopg2 import sql


def connect_to_db(params):
    try:
        conn = psycopg2.connect(
            host=params["host"],
            port=params["port"],
            dbname=params["dbname"],
            user=params["user"],
            password=params["password"],
        )
        return conn
    except Exception as e:
        print(json.dumps({"error": f"Connection error: {e}"}))
        sys.exit(1)


def insert_user(conn, socket, username, user_password):
    try:
        with conn.cursor() as cur:
            cur.execute(
                sql.SQL(
                    "INSERT INTO users (socket, username, user_password) VALUES (%s, %s, %s)"
                ),
                (socket, username, user_password),
            )
            conn.commit()
            print(
                json.dumps(
                    {
                        "status": "success",
                        "message": f"User {username} uploaded",
                    }
                )
            )
    except Exception as e:
        print(json.dumps({"error": f"Insert error: {e}"}))


def select_users(conn):
    try:
        with conn.cursor() as cur:
            cur.execute("SELECT * FROM users")
            rows = cur.fetchall()
            print(json.dumps({"users": rows}))
    except Exception as e:
        print(json.dumps({"error": f"Select error: {e}"}))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(json.dumps({"error": "Have no params"}))
        sys.exit(1)

    params = json.loads(sys.argv[1])
    action = params.get("action")

    conn = connect_to_db(params)
    if not conn:
        sys.exit(1)

    try:
        if action == "insert":
            insert_user(
                conn, params["socket"], params["username"], params["user_password"]
            )
        elif action == "select":
            select_users(conn)
        else:
            print(json.dumps({"error": "Unknown action"}))
    finally:
        conn.close()
