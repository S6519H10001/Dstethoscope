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

print(random.random())
app = Flask(__name__)
CORS(app)  # Allow cross-origin requests

# MQTT broker details
broker = "mqtt.eclipseprojects.io"

port = 1883

@app.route('/audio', methods=['GET'])
def generate_sine_wave():
    frequency = random.randint(10000, 15000)  # Hz
    duration = 10   # seconds
    sample_rate = 100  # samples per second

    t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
    sine_wave = (100 * np.sin(2 * np.pi * frequency * t)).tolist()

    return jsonify({"time": t.tolist(), "amplitude": sine_wave})
# Callback when a message is received
def on_message(client, userdata, msg):
    print(f"Received message===================================: {msg.payload} on topic {msg.topic}")

# Callback when connected to the broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
        client.subscribe("test/audio/jsonStringTest")
        client.subscribe("test/topicTar/temp")
    else:
        print(f"Connection failed with code {rc}")
def mqtt_thread():
    # Create MQTT client
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    # Connect to broker
    print("Connecting to broker...")
    client.connect(broker, port)
    client.loop_forever()

mqtt_thread = threading.Thread(target=mqtt_thread)
mqtt_thread.start()
if __name__ == '__main__':
    app.run(host="127.0.0.1", port=5000, debug=True)
