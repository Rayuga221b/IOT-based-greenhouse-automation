# 🌱 Smart IoT Greenhouse Automation System (ESP32 + CoAP)

A complete automated greenhouse environment built using **ESP32**, **CoAP protocol**, and environmental sensors, fully simulated using **Wokwi**.

---

## Overview

This project implements a **Smart Greenhouse Automation System** capable of monitoring and controlling plant-growing conditions using:

- 🌡️ DHT22 – Temperature & humidity  
- 🌱 Soil Moisture Sensor (Potentiometer simulation)  
- 🔆 LDR – Light detection  
- 💨 Fan Control  
- 🚿 Irrigation Pump Control  
- 🔄 Servo-controlled Ventilation System  
- 🌐 Built-in CoAP Server for remote monitoring & control  

Everything works autonomously with an “Auto Mode”, and can also be controlled remotely using CoAP commands.

---

## 🛠 Features

### ✔ Environment Monitoring
- Temperature  
- Humidity  
- Soil moisture  
- Light intensity  

### ✔ Automatic Climate Control
- Fan activates when temperature/humidity cross set thresholds  
- Pump waters plants when soil moisture is low  
- Servo vent opens/closes depending on temperature    

### ✔ CoAP Remote Control

| Endpoint   | Method | Description               |
|-----------|--------|---------------------------|
| `/sensors` | GET    | Fetch all sensor data     |
| `/fan`     | POST   | Turn fan ON/OFF           |
| `/pump`    | POST   | Turn pump ON/OFF          |
| `/mode`    | POST   | Switch AUTO/MANUAL mode   |

---

## 📷 Screenshots


```md
![Indicators](/screenshot-1.png)
![Simulation](/screenshot-2.png)
![Serial Output](/screenshot-3.png)


