const serverUrl = "ws://192.168.132.36:8000/audio"; // 🔗 เปลี่ยนเป็น IP ของเซิร์ฟเวอร์ FastAPI
let ws;
let audioContext;
let scriptProcessor;
let audioQueue = [];

document.getElementById("connect").addEventListener("click", startStreaming);
document.getElementById("disconnect").addEventListener("click", stopStreaming);

function startStreaming() {
    ws = new WebSocket(serverUrl);
    ws.binaryType = "arraybuffer"; // 📡 รับข้อมูลเป็น binary

    ws.onopen = () => {
        document.getElementById("status").innerText = "Status: Connected ✅";
        document.getElementById("connect").disabled = true;
        document.getElementById("disconnect").disabled = false;
        setupAudio();
    };

    ws.onmessage = (event) => {
        audioQueue.push(new Int16Array(event.data)); // 📥 เก็บข้อมูลเสียงใน queue
    };

    ws.onclose = () => {
        document.getElementById("status").innerText = "Status: Disconnected ❌";
        document.getElementById("connect").disabled = false;
        document.getElementById("disconnect").disabled = true;
        stopAudio();
    };
}

function stopStreaming() {
    if (ws) ws.close();
}

// 🎵 ตั้งค่า Web Audio API
function setupAudio() {
    audioContext = new AudioContext({ sampleRate: 16000 }); // 🔊 ต้องตรงกับ ESP32
    scriptProcessor = audioContext.createScriptProcessor(4096, 1, 1);
    
    scriptProcessor.onaudioprocess = (event) => {
        const output = event.outputBuffer.getChannelData(0);
        for (let i = 0; i < output.length; i++) {
            output[i] = audioQueue.length > 0 ? audioQueue.shift() / 32768.0 : 0;
        }
    };

    scriptProcessor.connect(audioContext.destination);
    audioContext.resume();
}

// 🛑 ปิด Web Audio API
function stopAudio() {
    if (scriptProcessor) scriptProcessor.disconnect();
    if (audioContext) audioContext.close();
    audioQueue = [];
}
