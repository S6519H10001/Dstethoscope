import websocket

try:
    ws = websocket.WebSocket()
    ws.connect("ws://127.0.0.1:8000/audio")  # ใช้ 127.0.0.1 เพราะรันบนเครื่องเดียวกัน
    print("✅ WebSocket Connected!")
    ws.close()
except Exception as e:
    print(f"❌ WebSocket Connection Failed: {e}")

