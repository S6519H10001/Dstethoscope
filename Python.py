import os
from flask import Flask
from flask_socketio import SocketIO

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")

# ✅ ใช้พอร์ตจาก Render (ค่าเริ่มต้นเป็น 5000)
port = int(os.environ.get("PORT", 5000))

@socketio.on("connect")
def handle_connect():
    print("✅ WebSocket Client Connected")

if __name__ == "__main__":
    print(f"🚀 Running WebSocket Server on port {port}...")
    socketio.run(app, host="0.0.0.0", port=port, certfile="cert.pem", keyfile="key.pem")
