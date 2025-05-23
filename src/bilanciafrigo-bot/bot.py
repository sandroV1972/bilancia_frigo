
import json
import paho.mqtt.client as mqtt
from telegram import Bot
from config import BOT_TOKEN, MQTT_BROKER, MQTT_PORT, MQTT_TOPIC

bot = Bot(token=BOT_TOKEN)
bilance_file = "bilance.json"

def load_registrazioni():
    try:
        with open(bilance_file, "r") as f:
            return json.load(f)
    except Exception:
        return {}

def salva_registrazioni(data):
    with open(bilance_file, "w") as f:
        json.dump(data, f)

def handle_mqtt(client, userdata, msg):
    text = msg.payload.decode("utf-8")
    print(f"Ricevuto: {text}")
    if text.startswith("[") and "]" in text:
        id_bilancia = text.split("]")[0][1:]
        messaggio = text.split("]")[1].strip()
        utenti = load_registrazioni()
        if id_bilancia in utenti:
            for chat_id in utenti[id_bilancia]:
                try:
                    bot.send_message(chat_id=chat_id, text=messaggio)
                except Exception as e:
                    print(f"Errore invio a {chat_id}: {e}")
        else:
            print(f"Nessun utente registrato per {id_bilancia}")

client = mqtt.Client()
client.on_message = handle_mqtt

def start():
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.subscribe(MQTT_TOPIC)
    client.loop_forever()

if __name__ == "__main__":
    start()
