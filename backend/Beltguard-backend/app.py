import os
from flask import Flask, jsonify, request
from flask_cors import CORS
from dotenv import load_dotenv
from influxdb_client import InfluxDBClient

load_dotenv()

INFLUX_URL = os.getenv("INFLUX_URL")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")
INFLUX_ORG = os.getenv("INFLUX_ORG")
INFLUX_BUCKET = os.getenv("INFLUX_BUCKET")

influx_client = InfluxDBClient(
    url=INFLUX_URL,
    token=INFLUX_TOKEN,
    org=INFLUX_ORG
)

query_api = influx_client.query_api()

app = Flask(__name__)
CORS(app)


@app.route("/")
def home():
    return "BeltGuard Backend is running!"


@app.route("/api/latest")
def latest():
    query = f'''
    from(bucket: "{INFLUX_BUCKET}")
      |> range(start: -24h)
      |> filter(fn: (r) => r["_measurement"] == "belt_telemetry")
      |> last()
    '''

    tables = query_api.query(query)

    result = {}

    for table in tables:
        for record in table.records:
            result[record.get_field()] = record.get_value()

            # Add tags
            result["node_id"] = record.values.get("node_id")
            result["joint_id"] = record.values.get("joint_id")

            # Add timestamp
            result["timestamp"] = record.get_time().isoformat()

    return jsonify(result)


@app.route("/api/history")
def history():
    joint = request.args.get("joint", "J-01")
    time_range = request.args.get("range", "5m")

    query = f'''
    from(bucket: "{INFLUX_BUCKET}")
      |> range(start: -{time_range})
      |> filter(fn: (r) => r["_measurement"] == "belt_telemetry")
      |> filter(fn: (r) => r["joint_id"] == "{joint}")
    '''

    tables = query_api.query(query)

    readings = {}

    for table in tables:
        for record in table.records:
            timestamp = record.get_time().isoformat()

            if timestamp not in readings:
                readings[timestamp] = {
                    "timestamp": timestamp
                }

            readings[timestamp][record.get_field()] = record.get_value()

    return jsonify(list(readings.values()))

@app.route("/api/alerts")
def alerts():
    query = f'''
    from(bucket: "{INFLUX_BUCKET}")
      |> range(start: -24h)
      |> filter(fn: (r) => r["_measurement"] == "belt_anomaly")
      |> pivot(
          rowKey: ["_time"],
          columnKey: ["_field"],
          valueColumn: "_value"
      )
      |> filter(fn: (r) =>
          r.risk_level == "WARNING" or
          r.risk_level == "CRITICAL" or
          r.risk_level == "SENSOR_FAULT"
      )
    '''

    tables = query_api.query(query)

    result = []

    for table in tables:
        for record in table.records:
            result.append({
                "timestamp": record.get_time().isoformat(),
                "risk_level": record.values.get("risk_level"),
                "anomaly_score": record.values.get("anomaly_score")
            })

    return jsonify(result)

if __name__ == "__main__":
    app.run(debug=True)