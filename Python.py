import paho.mqtt.client as mqtt
from flask import Flask, request, jsonify
from flask_cors import CORS
import threading
import json

app = Flask(__name__)
CORS(app)

# ✅ ตัวแปรเก็บข้อมูลเสียง
audio_data = {"amplitude": [], "time": []}

# ✅ API รองรับทั้ง GET และ POST
@app.route('/audio', methods=['GET', 'POST'])
def audio():
    if request.method == 'GET':
        return jsonify(audio_data)  # ส่งข้อมูลเสียงกลับไป

    elif request.method == 'POST':
        data = request.get_json()
        if data and "amplitude" in data and "time" in data:
            audio_data["amplitude"] = data["amplitude"]
            audio_data["time"] = data["time"]
            return jsonify({"message": "Data received"}), 200
        else:
            return jsonify({"error": "Invalid data format"}), 400

# ✅ ตั้งค่า MQTT
broker = "mqtt.eclipseprojects.io"
port = 1883

def on_message(client, userdata, msg):
    print(f"Received message: {msg.payload.decode()} on topic {msg.topic}")

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
        client.subscribe("test/audio/jsonStringTest")
        client.subscribe("test/topicTar/temp")
    else:
        print(f"Connection failed with code {rc}")

def mqtt_thread():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    print("Connecting to broker...")
    client.connect(broker, port)
    client.loop_forever()

# ✅ รัน MQTT ใน Thread (Daemon)
mqtt_thread = threading.Thread(target=mqtt_thread, daemon=True)
mqtt_thread.start()

# ✅ รัน API แบบ Production ด้วย `waitress`
if __name__ == '__main__':
    from waitress import serve
    print("🚀 Running API in Production Mode...")
    serve(app, host="0.0.0.0", port=5000)
