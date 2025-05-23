import os
import paho.mqtt.client as mqtt

MQTT_BROKER = os.getenv("MQTT_BROKER")
MQTT_PORT = int(os.getenv("MQTT_PORT", 1883))
MQTT_TOPIC = os.getenv("MQTT_TOPIC")

client = mqtt.Client()

def publish_message(device_id, message):
    topic = f"bilancia/{device_id}"
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.publish(topic, message)
    client.disconnect()
