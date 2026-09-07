import os
from dotenv import load_dotenv
from influxdb_client import InfluxDBClient

load_dotenv()

url = os.getenv("INFLUX_URL")
token = os.getenv("INFLUX_TOKEN")
org = os.getenv("INFLUX_ORG")

client = InfluxDBClient(
    url=url,
    token=token,
    org=org
)

print("Connected to InfluxDB!")

client.close()