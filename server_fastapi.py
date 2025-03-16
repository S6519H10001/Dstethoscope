from fastapi import FastAPI, WebSocket
from fastapi.middleware.cors import CORSMiddleware
import wave
import asyncio
import os

app = FastAPI()

# ✅ เพิ่ม CORS Middleware เพื่อให้ ESP32 เชื่อมต่อได้
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# ✅ เช็คให้แน่ใจว่าไฟล์เก่าไม่ได้ถูกใช้งาน
AUDIO_FILE = "fastapi_audio.wav"
if os.path.exists(AUDIO_FILE):
    os.remove(AUDIO_FILE)

@app.get("/")
async def root():
    return {"message": "FastAPI is running!"}

# ✅ WebSocket สำหรับ Audio Streaming
@app.websocket("/audio")
async def websocket_audio(websocket: WebSocket):
    await websocket.accept()
    print("🎤 Client connected for Audio Streaming")

    try:
        with wave.open(AUDIO_FILE, "wb") as wav_file:
            wav_file.setnchannels(2)
            wav_file.setsampwidth(4)  # 32-bit PCM
            wav_file.setframerate(16000)

            while True:
                try:
                    data = await websocket.receive_bytes()
                    wav_file.writeframes(data)
                    print(f"🎧 Received {len(data)} bytes of audio")
                except asyncio.CancelledError:
                    print("❌ WebSocket Cancelled")
                    break
                except Exception as e:
                    print(f"⚠️ Error receiving audio: {e}")
                    break

    except Exception as e:
        print(f"⚠️ Error opening WAV file: {e}")

    finally:
        print("❌ Audio Streaming Disconnected")
        await websocket.close()

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8080)
