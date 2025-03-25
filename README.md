🌱 Smart Plant Monitoring System with ESP32
Created by Naveen, Anuub, and Bharath from Vels University

An IoT-based plant monitoring system that uses ESP32, sensors, and AI to provide real-time plant health analysis and automated care.

🚀 Features
Real-time Monitoring
📊 Tracks soil moisture, temperature, humidity, light levels, and water levels.

Automated Plant Care
💧 Controls water pump and mist maker based on sensor data.

AI-Powered Insights
🧠 Uses Groq API for plant health summaries and chat-based queries.

Web Interface
🌐 Built-in dashboard to monitor and control the system remotely.

Alerts & Notifications
🔔 Visual/audio alerts for critical conditions (low moisture/water).

🛠 Hardware Components
Component	Usage
ESP32 (38-pin)	Main microcontroller
LDR Sensor	Light intensity measurement
DHT11	Temperature & humidity sensor
BMP180	Pressure sensor
Capacitive Soil Sensor	Soil moisture detection
Water Level Sensor	Tank water level monitoring
Relay Module	Controls pump & mist maker
DFPlayer Mini	Audio alerts & responses
RGB LED	Visual status indicators
Buzzer	Alert sounds
🔌 Circuit Diagram
Circuit Diagram
(Replace with your actual diagram image)

📝 Setup Guide
1. Prerequisites
Arduino IDE or PlatformIO (recommended)

ESP32 Board Support

Libraries:

Copy
Adafruit BMP085, DHT sensor, ArduinoJson, ESPAsyncWebServer, DFRobotDFPlayerMini
2. Installation
Clone this repository:

bash
Copy
git clone https://github.com/yourusername/smart-plant.git
Update credentials in main.cpp:

cpp
Copy
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const String groqKey = "YOUR_GROQ_API_KEY";
Upload the code to ESP32.

3. Access the Web Interface
Connect to the ESP32’s WiFi AP or ensure it’s on your local network.

Open a browser and navigate to:

Copy
http://[ESP32_IP_ADDRESS]
🤖 AI Integration
The system uses Groq API (LLM) to:

Generate plant health summaries every 10 seconds.

Answer user questions like "Is my plant healthy?" using real-time sensor data.

Respond concisely (15-word limit) without follow-up questions.

Example AI Interaction:
👤 "Should I water the plant?"
🤖 "Soil moisture is 30%. Water needed."

🌟 Highlights
✅ Multi-sensor integration (LDR, DHT11, BMP180, capacitive soil)
✅ Automated responses (pump/mist control + alerts)
✅ Low-power design with deep sleep support (optional)
✅ WebSocket for real-time dashboard updates

📊 Sample Output
Serial Monitor:

Copy
[WiFi] Connected! IP: 192.168.1.100  
[LDR] Light Level: 65%  
[Soil] Moisture: 45%  
[AI] "Plant is healthy. Optimal conditions."  
Web Interface:
Web Dashboard

📚 Documentation
ESP32 Pinout

Groq API Docs

Sensor Datasheets (Upload yours)

🙏 Credits
Developed by:

Naveen

Anuub

Bharath

Guided by Vels University.

📜 License
MIT License - See LICENSE for details.

🌿 Keep your plants alive smarter!

🎨 Tips to Enhance Your README:
Replace placeholder images with actual project photos/diagrams.

Add a "Demo Video" section (link to YouTube).

Include troubleshooting FAQs if needed.

Add a "Future Upgrades" section for planned features.
