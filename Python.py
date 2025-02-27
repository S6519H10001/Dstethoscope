from flask import Flask
from flask_socketio import SocketIO
import base64
import numpy as np
import os  # ✅ Import os เพื่อดึงค่าพอร์ตจาก Render

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")

# ✅ รับข้อมูลเสียงจาก ESP32
@socketio.on("message")
def handle_audio(data):
    try:
        audio_base64 = data["audio"]
        audio_bytes = base64.b64decode(audio_base64)
        audio_array = np.frombuffer(audio_bytes, dtype=np.int16)

        print(f"✅ Received {len(audio_array)} samples of audio data")

    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    port = int(os.environ.get("PORT", 8080))  # ✅ ใช้พอร์ตจาก Render
    print(f"🚀 Running WebSocket Server on port {port}...")
    socketio.run(app, host="0.0.0.0", port=5000, certfile="cert.pem", keyfile="key.pem")
