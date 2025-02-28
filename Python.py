from flask import Flask
from flask_socketio import SocketIO

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")

# ✅ เพิ่ม Route "/"
@app.route("/")
def home():
    return "WebSocket Server is Running!", 200

@socketio.on("connect")
def handle_connect():
    print("✅ WebSocket Client Connected")

if __name__ == "__main__":
    import os
    port = int(os.environ.get("PORT", 8080))  # ✅ ใช้พอร์ตที่ Render กำหนด
    print(f"🚀 Running WebSocket Server on port {port}...")
    socketio.run(app, host="0.0.0.0", port=port)
