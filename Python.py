from ast import Subscript
import paho.mqtt.client as mqtt
from fastapi import FastAPI, HTTPException
from flask import Flask, cli, jsonify
from flask_cors import CORS
import time
import uvicorn 
import threading
from datetime import datetime
import json
import threading
import numpy as np
import random
from flask import Flask, request, jsonify
from flask_cors import CORS
import numpy as np
import random
import threading
import paho.mqtt.client as mqtt

app = Flask(__name__)
CORS(app)

# ✅ เพิ่มตัวแปรเก็บข้อมูลเสียง
audio_data = {"amplitude": [], "time": []}

# ✅ `/audio` รองรับทั้ง `GET` และ `POST`
@app.route('/audio', methods=['GET', 'POST'])
def audio():
    if request.method == 'GET':
        return jsonify(audio_data)  # ส่งข้อมูลเสียงที่มีอยู่กลับไป

    elif request.method == 'POST':
        data = request.json
        if "amplitude" in data and "time" in data:
            audio_data["amplitude"] = data["amplitude"]
            audio_data["time"] = data["time"]
            return jsonify({"message": "Data received"}), 200
        else:
            return jsonify({"error": "Invalid data"}), 400

# ✅ MQTT ตั้งค่าเหมือนเดิม
broker = "mqtt.eclipseprojects.io"
port = 1883

def on_message(client, userdata, msg):
    print(f"Received message: {msg.payload} on topic {msg.topic}")

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

mqtt_thread = threading.Thread(target=mqtt_thread)
mqtt_thread.start()

# ✅ แก้ไขให้ API ใช้งานได้จาก ESP32
if __name__ == '__main__':
    from waitress import serve  # ใช้ waitress แทน werkzeug
    print("🚀 Running API in Production Mode...")
    serve(app, host="0.0.0.0", port=5000)
