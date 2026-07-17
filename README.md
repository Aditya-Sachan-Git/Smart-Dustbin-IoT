# 🗑️ Smart Dustbin IoT

A Smart Dustbin built using the ESP8266 microcontroller that automatically detects, classifies, and segregates waste into **wet** and **dry** categories while providing real-time monitoring through a web dashboard.

The system integrates multiple sensors for waste detection, moisture-based classification, garbage level monitoring, and fire detection to improve waste management efficiency.

---

## Features

- Automatic Waste Detection
- Wet/Dry Waste Classification
- Servo-controlled Waste Segregation
- Bin Level Monitoring
- Fire Hazard Detection
- Real-time IoT Dashboard
- Wi-Fi Connectivity
- Automatic Notifications

---

## Hardware Components

- ESP8266 Wi-Fi Module
- Ultrasonic Sensor
- Soil Moisture Sensor
- Servo Motor
- IR Sensors
- Flame Sensor
- Breadboard
- Jumper Wires

---

## Software Used

- Arduino IDE
- ESP8266 Wi-Fi Libraries
- Embedded C++
- Web Dashboard

---

## Working Principle

1. The ultrasonic sensor detects when waste is placed near the dustbin.
2. The soil moisture sensor measures the moisture content of the waste.
3. The ESP8266 classifies the waste as **wet** or **dry**.
4. The servo motor rotates the flap to direct the waste into the correct compartment.
5. IR sensors monitor the fill level of both bins.
6. The flame sensor continuously checks for fire hazards.
7. All sensor data is transmitted over Wi-Fi to a web dashboard for remote monitoring.

---

## System Architecture

```
Ultrasonic Sensor
        │
        ▼
ESP8266 Controller
        │
 ┌──────┼──────────────┐
 │      │              │
 ▼      ▼              ▼
Soil   Servo      Wi-Fi Module
Sensor  Motor          │
 │                     ▼
 ▼              Web Dashboard
IR Sensors
 │
 ▼
Flame Sensor
```

---

## Technologies Used

- C++
- Arduino IDE
- ESP8266
- IoT
- Wi-Fi
- Embedded Systems

---

## Project Structure

```
smart-dustbin-iot/
│
├── firmware/
│   └── smart_dustbin.ino
│
├── images/
│   ├── block_diagram.png
│   ├── prototype.jpg
│   └── dashboard.png
│
├── report/
│   └── Smart_Dustbin_Report.pdf
│
├── README.md
├── LICENSE
└── .gitignore
```

---

## How It Works

- Detects incoming waste using an ultrasonic sensor.
- Measures moisture to classify waste as wet or dry.
- Rotates a servo-controlled flap to the appropriate compartment.
- Continuously monitors garbage levels using IR sensors.
- Detects fire hazards using a flame sensor.
- Uploads live sensor data to a web interface through the ESP8266.

---

## Future Improvements

- Mobile application integration
- MQTT-based communication
- GPS-enabled smart waste collection
- Solar-powered operation
- AI-based waste recognition
- Cloud analytics
- Multi-bin management

---

## Learning Outcomes

This project helped me understand:

- Internet of Things (IoT)
- Embedded Systems Programming
- ESP8266 Development
- Sensor Interfacing
- Servo Motor Control
- Real-time Monitoring
- Wi-Fi Communication
- Automation Systems

