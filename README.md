# Smart Dustbin IoT

An ESP32/ESP8266-based Smart Dustbin that automatically detects incoming waste, captures an image, communicates with a Flask backend for AI-based classification, and controls a servo motor to direct waste into the appropriate compartment.

This project demonstrates the integration of embedded systems, IoT communication, computer vision, and automation.

---

## Features

- Ultrasonic object detection
- Automatic image capture
- Wi-Fi communication
- HTTP API integration
- AI-based waste classification
- Servo-controlled waste segregation
- Automatic reset mechanism
- Real-time backend communication

---

## Hardware

- ESP32-CAM / ESP8266
- HC-SR04 Ultrasonic Sensors
- Servo Motor
- 5V Power Supply

---

## Software

- Arduino IDE
- ESP32 Libraries
- HTTPClient
- Wi-Fi Library
- Flask Backend
- TensorFlow/Keras Model

---

## Working Principle

1. Detect object using ultrasonic sensor.
2. Capture waste image.
3. Send image to Flask server.
4. Backend predicts waste category.
5. Receive prediction.
6. Rotate servo accordingly.
7. Return servo to neutral position.

---

## Firmware Features

- Wi-Fi Connectivity
- HTTP POST Requests
- Camera Control
- Servo Automation
- Distance Monitoring
- Automatic Reset Logic

---

## Technologies

- C++
- Arduino IDE
- ESP32
- HTTP
- REST API
- Wi-Fi

---

## Future Improvements

- MQTT Communication
- OTA Firmware Updates
- Secure Wi-Fi Provisioning
- On-device TensorFlow Lite
- Battery Operation
- Mobile Application
- Cloud Dashboard
- Multi-bin Deployment

---

## Learning Outcomes

This project helped me understand:

- Embedded Programming
- IoT Communication
- REST APIs
- ESP32 Development
- Sensor Integration
- Servo Control
- Edge AI Concepts
- Hardware-Software Integration
