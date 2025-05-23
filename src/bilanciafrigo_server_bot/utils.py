import json
import os

USERS_FILE = "users.json"

def load_users():
    if os.path.exists(USERS_FILE):
        with open(USERS_FILE, "r") as f:
            return json.load(f)
    return {}

def save_user(device_id, chat_id):
    users = load_users()
    users[device_id] = chat_id
    with open(USERS_FILE, "w") as f:
        json.dump(users, f)
