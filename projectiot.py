import time
import json
from paho.mqtt import client as mqtt_client
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS

# ============ MQTT Configuration ============
broker = '10.13.40.21'   # Your MQTT broker IP (e.g. Windows PC running Mosquitto)
port = 1883
mqtt_topics = [
    "esp32/temp",
    "esp32/tds_voltage",
    "esp32/ec",
    "esp32/turbidity_voltage",
    "esp32/turbidity_percent"
]
client_id = f'mqtt-influx-{int(time.time())}'

# ============ InfluxDB Configuration ============
influx_url = "http://localhost:8086"         # InfluxDB server URL
influx_token = "AfuGjubmyG3yLJ7F58jh6ppMGIcH4PEAfomDo_GzZM6WxXRk4ZhGYmLLoWC8cnMkwRgGsgBJJOg5NiVQ_o-llA=="         # InfluxDB API Token
influx_org = "National Textile University"                      # Your InfluxDB org name
influx_bucket = "final-iot"              # Your InfluxDB bucket

# ============ Initialize InfluxDB Client ============
influx_client = InfluxDBClient(
    url=influx_url,
    token=influx_token,
    org=influx_org
)
write_api = influx_client.write_api(write_options=SYNCHRONOUS)

# ============ Callback Functions ============
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Connected to MQTT Broker!")
        for topic in mqtt_topics:
            client.subscribe(topic)
            print(f"📥 Subscribed to topic: {topic}")
    else:
        print(f"❌ Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    try:
        topic = msg.topic
        payload = msg.payload.decode()
        value = float(payload)

        # Create InfluxDB point
        point = Point("water_quality") \
            .tag("device", "esp32") \
            .field(topic.split("/")[-1], value) \
            .time(time.time_ns())

        write_api.write(bucket=influx_bucket, org=influx_org, record=point)
        print(f"📤 Written to InfluxDB: {topic} -> {value}")

    except Exception as e:
        print(f"⚠️ Error processing message: {e}")

# ============ Main ============
def run():
    mqtt_client_instance = mqtt_client.Client(client_id)
    mqtt_client_instance.on_connect = on_connect
    mqtt_client_instance.on_message = on_message

    mqtt_client_instance.connect(broker, port)
    mqtt_client_instance.loop_forever()

if __name__ == '__main__':
    run()
