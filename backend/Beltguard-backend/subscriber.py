import os
import json
import paho.mqtt.client as mqtt
from dotenv import load_dotenv
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS

load_dotenv()

BROKER = os.getenv("MQTT_BROKER")
PORT = int(os.getenv("MQTT_PORT"))
TOPIC = os.getenv("MQTT_TOPIC")

INFLUX_URL = os.getenv("INFLUX_URL")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")
INFLUX_ORG = os.getenv("INFLUX_ORG")
INFLUX_BUCKET = os.getenv("INFLUX_BUCKET")

influx_client = InfluxDBClient(
    url=INFLUX_URL,
    token=INFLUX_TOKEN,
    org=INFLUX_ORG
)

write_api = influx_client.write_api(write_options=SYNCHRONOUS)

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")

        client.subscribe(TOPIC)
        print(f"Subscribed to: {TOPIC}")
    else:
        print("Connection failed. Error code:", rc)


def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())

        print("\nMessage received!")
        print("Topic:", msg.topic)
        print("Data:", data)

        point = (
            Point("belt_telemetry")
            .tag("node_id", data["node_id"])
            .tag("joint_id", data["joint_id"])
            .field("vibration_rms", data["vibration_rms"])
            .field("vibration_peak", data["vibration_peak"])
            .field("temp_surface_c", data["temp_surface_c"])
            .field("temp_bearing_c", data["temp_bearing_c"])
            .field("belt_speed_mps", data["belt_speed_mps"])
            .field("load_kg", data["load_kg"])
            .field("ir_offset_mm", data["ir_offset_mm"])
            .field("sensor_fault_flags", data["sensor_fault_flags"])
            .field("anomaly_score", data["anomaly_score"])
            .field("risk_level", data["risk_level"])
        )

        write_api.write(
            bucket=INFLUX_BUCKET,
            org=INFLUX_ORG,
            record=point
        )

        print("Data written to InfluxDB!")

    except json.JSONDecodeError:
        print("Received invalid JSON")

    except Exception as e:
        print("Error writing to InfluxDB:", e)
 

client = mqtt.Client()

client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, 60)

print("Starting MQTT subscriber...")

client.loop_forever()