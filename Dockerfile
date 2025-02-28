# ใช้ Mosquitto MQTT Broker
FROM eclipse-mosquitto:latest

# เปิดพอร์ต MQTT
EXPOSE 1883 9001

# คัดลอกไฟล์ Config
COPY mosquitto.conf /mosquitto/config/mosquitto.conf

# รัน Mosquitto
CMD ["mosquitto", "-c", "/mosquitto/config/mosquitto.conf", "-v"]
