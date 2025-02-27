from flask import Flask
from flask_socketio import SocketIO
import base64
import numpy as np

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")

# ✅ รับข้อมูลเสียงจาก ESP32 (ไม่ส่งผลลัพธ์กลับไป)
@socketio.on("message")
def handle_audio(data):
    try:
        audio_base64 = data["audio"]  # รับข้อมูลเสียงแบบ Base64
        audio_bytes = base64.b64decode(audio_base64)  # แปลงเป็น bytes

        # ✅ แปลงข้อมูลเสียงเป็น NumPy Array (16-bit PCM)
        audio_array = np.frombuffer(audio_bytes, dtype=np.int16)

        print(f"✅ Received {len(audio_array)} samples of audio data")

        # ✅ สามารถนำ `audio_array` ไปบันทึกหรือวิเคราะห์เพิ่มเติมได้ที่นี่
        # ตัวอย่าง: np.save("audio_data.npy", audio_array)  # บันทึกไฟล์เสียงเป็น NumPy Array

    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    print("🚀 Running WebSocket Server...")
    socketio.run(app, host="0.0.0.0", port=5000)
