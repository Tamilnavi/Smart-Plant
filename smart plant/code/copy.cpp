#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>
#include <DFRobotDFPlayerMini.h>
#include <RTClib.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <ESP32Time.h>
#include <U8g2lib.h>

// Hardware Configuration
#define DHTPIN 14
#define DHTTYPE DHT11
#define TOUCH_PIN 15
#define SOIL_MOISTURE_PIN 34
#define WATER_LEVEL_PIN 35
#define LIGHT_SENSOR_PIN 33
#define BUZZER_PIN 5
#define PUMP_RELAY 32
#define MIST_RELAY 13
#define RGB_RED 25
#define RGB_GREEN 26
#define RGB_BLUE 27
#define DF_BUSY 2

// Display Setup
Adafruit_SSD1306 display(128, 32, &Wire, -1);
Adafruit_BMP085 bmp;
DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;
DFRobotDFPlayerMini dfPlayer;
ESP32Time rtcSoftware;

// Initialize OLED display using I2C
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// WiFi and API
const char* ssid = "NAVEEN";
const char* password = "naveen2006";
const String openWeatherKey = "075411f1a6bfd338e171f4c6c49a2888";
// const String geminiKey = "AIzaSyD9JB_qLisa_UycsmiL9Y0qV02k3avj3Sk";
// In global variables section
const String Key = "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"; // Replace with your Groq API key
const String model = "llama-3.3-70b-versatile"; // or "llama2-70b-4096" llama3-8b-8192
// System State
struct SystemState {
  // Sensor Data
  float temp = 0;
  float humidity = 0;
  float pressure = 0;
  int soilMoisture = 0;
  int waterLevel = 0;
  int lightLevel = 0;
  
  // Device States
  bool pumpActive = false;
  bool mistActive = false;
  bool pumpInt = true; 
  bool waterInt = true;
  bool buzy = false;
  bool weatherInt = true;
  bool reportInt = true;
  unsigned long pumpStart = 0;
  unsigned long mistStart = 0;
  unsigned long waterStart = 0;

  
  // Audio States
  bool isPlaying = false;
  unsigned long soundEnd = 0;
  
  // Time Management
  DateTime currentTime;
  unsigned long lastHourBeep = 0;
  
  // AI Data
  String aiSummary = "";
  String weatherDesc = "";
  String report = "";
  String weatherIcon = "";
  String slide = "";
  float extTemp = 0;
};
int ldr = 0;
SystemState state;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
bool internetAvailable = false;
char greetingmode = 'N';

// PWM Configuration
const int pwmFreq = 5000;
const int pwmResolution = 8;
const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;

// Prototypes
void initializeHardware();
void connectWiFi();
void initializeWebServer();
void initializeSensors();
void initializeAudio();
void updateTime();
void readSensors();
void handleAlerts();
void handleScheduledEvents();
void handleAIProcessing();
void updateDisplay();
void handleManualTrigger();
void sendDataToClients();
void triggerAlert(uint8_t r, uint8_t g, uint8_t b, int sound);
void controlRGB(uint8_t r, uint8_t g, uint8_t b);
void getWeatherData();
String queryGemini(String prompt);
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                     AwsEventType type, void *arg, uint8_t *data, size_t len);
void disp(const char* txt);

const char webpageHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Smart Plant Monitor</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
      :root {
          --primary: #4E9F3D;
          --secondary: #2196F3;
          --background: #191A19;
      }

      body {
          font-family: Arial, sans-serif;
          margin: 0;
          padding: 20px;
          background: var(--background);
        
          
      }
      .topic{
        grid-column: span 2;
          width: 100%;
          display: flex;
          justify-content: center;
          background-color: #56ff78;
          border-radius: 10px;
          gap: 10px;
          margin-bottom: 20px;
          margin: 0 auto;
          
      }
      .custom-icon {
        font-size: 50px;  /* Adjust the size */
        color: green;     /* Change color */
        padding-left: 30px ;
        align-self: center;
        justify-content: right;
       
       
}

      .dashboard {
          /* max-width: 800px; */
          margin: 0 auto;
          display: grid;
          grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
          gap: 15px;
          height: 100vh;  /* 100% of the viewport height */
      }

      .card , .ai_card {
          background: #D8E9A8;
          border-radius: 10px;
          padding: 10px;
          box-shadow: 0 2px 5px rgba(0,0,0,0.1);
          margin-bottom: 20px;
          text-align: center;
          
      }
      /* .ai_card{
        grid-column: span 2;

      } */

      .sensor-grid {
          display: grid;
          grid-template-columns: repeat(2, 1fr);
          gap: 10px;
          height: 100vh;
            margin: 0;
            flex-direction: column;
      }

      .sensor-item {
          padding: 15px;
          background: linear-gradient(to bottom, rgb(0, 255, 21), rgba(255, 165, 0, 0));
          border-radius: 8px 8px;
          text-align: center;
          min-height: 80px;
          position: relative;  /* Needed for absolute positioning */
          padding: 15px;
          border: 2px solid rgba(0, 0, 0, 0.349);
          box-shadow: 0px 3px 8px rgba(0, 0, 0, 0.438);
          font-size: 24px;
          
      }
    
      #weather_box{
          grid-column: span 2;
          justify-items:center;
          text-align: center;
          
          
      }

     

      button {
          padding: 10px 20px;
          border: none;
          border-radius: 5px;
          background: var(--primary);
          color: white;
          cursor: pointer;
          transition: opacity 0.3s;
          min-width: fit-content;
          width: 30%;
      }

      button:active {
          opacity: 0.8;
      }
      button:hover{
          transition: .8s all;
          transform: skew(20deg, 0deg);
          
      }
      span{
        font-size: large;
        font-weight: bolder;
        display: flex;
        align-items: center;
        justify-content: center;
        text-align: center;
        min-height: 80px;
        color:rgb(10, 48, 0);
        
      }

      .alert {
          color: #d32f2f;
          animation: pulse 1s infinite;
      }
      #message{
        height: 30px;
        margin-right: 20px;
        border-radius: 8px;
      }

      #chat {
          margin-top: 20px;
          border-top: 1px solid #a39e9e;
          padding-top: 15px;
          position: relative;
          height: 100vh;
            margin: 0;
            display: flex;
            flex-direction: column;
      }
      #input{
        display: flex;
      }
      #chat-history{
        height: 85%;
        justify-content: center;
        background-color: #ffffff;
        border-radius: 8px;
        overflow-y: auto;
        text-align: justify;
        padding: 10px;
        
      }
        img {
    max-width: 100%; /* Ensure the image fits within its container */
    height: auto;    /* Maintain aspect ratio */
}

      @keyframes pulse {
          0% { opacity: 1; }
          50% { opacity: 0.5; }
          100% { opacity: 1; }
      }
  </style>
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css">


</head>
<body>
    <div class="topic">
        <h1><Big>S</Big>MART <big>P</big>LANT <small>v1.0</small></h1>
        <i class="fa-brands fa-github custom-icon" aria-label="Github document" onclick="window.open('https://github.com/Tamilnavi/Smart-Plant.git', '_blank');" style="cursor: pointer;"></i>
      </div>
  <div class="dashboard">
    
      <div class="card">
    <h2>🌱 Plant Sensors</h2>
    <div class="sensor-grid" id="sensors">
        <div class="sensor-item">🌡️ Temperature: <br> <b>--°C</b></div>
        <div class="sensor-item">💧 Humidity:<br> <b>--%</b></div>
        <div class="sensor-item" onclick="controlDevice('pump')">🌱 Soil Moisture:<br> <b>--</b></div>
        <div class="sensor-item">💦 Water Level:<br><b>--</b></div>
        <div class="sensor-item">🔆 Light Level:<br><b>--%</b></div>
        <div class="sensor-item">🎚️ Pressure:<br><b>-- hPa</b></div>
        <div class="sensor-item" style="grid-column: span 2;">🌤️ Weather:<br><b>--</b></div>
    </div>
</div>

      
      <!-- <div class="card">
          <h2>⚙️ Controls</h2>
          <div class="controls">
              <button onclick="controlDevice('pump')">💧 Water Pump</button>
              <!-- <button onclick="controlDevice('mist')">🌫️ Mist Maker</button> 
          </div>
          <div id="system-status" style="margin-top: 15px;"></div>
      </div> -->

      <div class="card">
          <h2>🤖 Smart Assistant</h2>
          <div id="ai-summary"></div>
          <div id="chat">
            <div id="input">
              <input type="text" id="message" placeholder="Ask the plant..." style="width: 70%;">
              <button onclick="sendMessage()">Send</button>
            </div>
              <div id="chat-history" style="margin-top: 10px;"></div>
          </div>
      </div>
  </div>
  
  <script>
    
      const ws = new WebSocket('ws://' + window.location.hostname + '/ws');
      
      ws.onopen = function() {
          console.log('WebSocket connected');
          updateStatus('Connected to device', 'green');
      };

      ws.onclose = function() {
          updateStatus('Connection lost', 'red');
      };

      ws.onmessage = function(event) {
          const data = JSON.parse(event.data);
          updateUI(data);
      };
      function updateUI(data) {
      const iconCode = data.icon; 
      const iconUrl = 'http://openweathermap.org/img/wn/'+ iconCode +'@2x.png';
    // Update only the inner text of each sensor item
    document.querySelector("#sensors").children[0].innerHTML = `🌡️ Temperature: <br> <span>${data.temp?.toFixed(1) ?? '--'}°C</span>`;
    document.querySelector("#sensors").children[1].innerHTML = `💧 Humidity:<br> <span>${data.humidity?.toFixed(1) ?? '--'}%</span>`;
    document.querySelector("#sensors").children[2].innerHTML = `🌱 Soil Moisture:<br> <span>${data.soil ?? '--'}</span><div id="system-status"></div>`;
    document.querySelector("#sensors").children[3].innerHTML = `💦 Water Level:<br><span>${data.water ?? '--'}</span>`;
    document.querySelector("#sensors").children[4].innerHTML = `🔆 Light Level:<br><span>${data.light ?? '--'}%</span>`;
    document.querySelector("#sensors").children[5].innerHTML = `🎚️ Pressure:<br><span>${data.pressure?.toFixed(1) ?? '--'} hPa</span>`;
    // document.querySelector("#sensors").children[6].innerHTML = `🌤️ Weather:<br><span>${data.weather ?? '--'}</span><br><span>${data.extmp ?? '--'} °C</span>`;
    document.querySelector("#sensors").children[6].innerHTML = `🌤️ Weather:  <img src="${iconUrl}" alt="${data.weather}" />${data.weather +"<br>"+ data.temp ?? '--'} °C</span>`;

    // Update AI summary
    document.getElementById('ai-summary').innerHTML = 
        data.aisummary ? `📝 ${data.aisummary}` : 'Waiting for sensor data...';

    // Update chat history
    if(data.aichat) {
        const chatHistory = document.getElementById('chat-history');
        chatHistory.innerHTML += `<div style="margin: 5px 0;">🤖: ${data.aichat}</div>`;
        chatHistory.scrollTop = chatHistory.scrollHeight;
    }
}


      function controlDevice(device) {
          fetch('/control', {
              method: 'POST',
              headers: {
                  'Content-Type': 'application/x-www-form-urlencoded',
              },
              body: `${device}=toggle`
          })
          .then(response => {
              if(!response.ok) throw Error('Control failed');
              updateStatus(`${device} toggled`, 'var(--primary)');
          })
          .catch(error => {
              updateStatus('Control failed', 'red');
              console.error(error);
          });
      }

      function sendMessage() {
          const input = document.getElementById('message');
          const message = input.value.trim();
          
          if(message) {
              ws.send(JSON.stringify({
                  type: 'chat',
                  message: message
              }));
              
              const chatHistory = document.getElementById('chat-history');
              chatHistory.innerHTML += `<div style="margin: 5px 0; font-weight:bold;"><big>You:</big> ${message}</div>`;
              input.value = '';
          }
      }

      function updateStatus(text, color) {
          const statusElement = document.getElementById('system-status');
          statusElement.innerHTML = text;
          statusElement.style.color = color;
          setTimeout(() => statusElement.innerHTML = '', 3000);
      }
      document.addEventListener("DOMContentLoaded", function () {
        updateUI({}); // Initializes with default values
    });
    
  </script>
  
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  u8g2.begin();
  initializeHardware();
  connectWiFi();
  initializeWebServer();
  initializeSensors();
  initializeAudio();
  getWeatherData();
  // display.clearDisplay();
  // display.print("Smart Plant v1.0");
  // display.display();
  disp("SMART PLANT v1.0");
  // dfPlayer.play(10);

  triggerAlert(0,255,0,10);
  
  delay(4000);
  // display.clearDisplay();
  
}

void loop() {
  static unsigned long lastUpdate = 0;
  
  ws.cleanupClients();
  updateTime();
  readSensors();
  handleAlerts();
  handleScheduledEvents();
  handleAIProcessing();
  updateDisplay();
  handleManualTrigger();
  
  // System heartbeat
  if(millis() - lastUpdate > 1000) {
    sendDataToClients();
    lastUpdate = millis();
  }
  state.buzy = digitalRead(DF_BUSY);
  delay(10);
}

void disp(const char* txt) {
  u8g2.clearBuffer();  // Clear the display buffer

  const uint8_t* fonts[] = {
    u8g2_font_10x20_tr,
    u8g2_font_7x14_tr,
    u8g2_font_6x10_tr,
    u8g2_font_5x8_tr
  };  // Font options in decreasing size

  int16_t maxWidth = 128;  // OLED width
  int16_t maxHeight = 32;  // OLED height
  int16_t lineHeight = 0;

  const uint8_t* selectedFont = fonts[0];
  int linesNeeded = 1;

  // Find the largest font that fits the display
  for (size_t i = 0; i < sizeof(fonts) / sizeof(fonts[0]); i++) {
    u8g2.setFont(fonts[i]);
    lineHeight = u8g2.getFontAscent() - u8g2.getFontDescent() + 1;

    // Simulate word wrapping to calculate lines needed
    char buffer[256];
    strncpy(buffer, txt, sizeof(buffer));
    char* word = strtok(buffer, " ");
    char line[128] = "";
    linesNeeded = 1;

    while (word) {
      char tempLine[128];
      snprintf(tempLine, sizeof(tempLine), "%s%s%s", line, strlen(line) > 0 ? " " : "", word);

      if (u8g2.getStrWidth(tempLine) > maxWidth) {
        linesNeeded++;
        strncpy(line, word, sizeof(line));
      } else {
        strncpy(line, tempLine, sizeof(line));
      }
      word = strtok(nullptr, " ");
    }

    if (linesNeeded * lineHeight <= maxHeight) {
      selectedFont = fonts[i];
      break;
    }
  }

  // Set the selected font
  u8g2.setFont(selectedFont);

  // Word wrapping and rendering
  char buffer[256];
  strncpy(buffer, txt, sizeof(buffer));
  char* word = strtok(buffer, " ");
  char line[128] = "";
  int16_t y = (maxHeight - (linesNeeded * lineHeight)) / 2 + lineHeight;

  while (word) {
    char tempLine[128];
    snprintf(tempLine, sizeof(tempLine), "%s%s%s", line, strlen(line) > 0 ? " " : "", word);

    if (u8g2.getStrWidth(tempLine) > maxWidth) {
      u8g2.drawStr((maxWidth - u8g2.getStrWidth(line)) / 2, y, line);
      y += lineHeight;
      strncpy(line, word, sizeof(line));
    } else {
      strncpy(line, tempLine, sizeof(line));
    }
    word = strtok(nullptr, " ");
  }

  if (strlen(line) > 0) {
    u8g2.drawStr((maxWidth - u8g2.getStrWidth(line)) / 2, y, line);
  }

  u8g2.sendBuffer();  // Update the display
}



// Hardware Initialization
void initializeHardware() {
  // GPIO Setup
  pinMode(PUMP_RELAY, OUTPUT);
  pinMode(MIST_RELAY, OUTPUT);
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  pinMode(TOUCH_PIN,INPUT);
  pinMode(DF_BUSY, INPUT);
  digitalWrite(PUMP_RELAY, LOW);
  digitalWrite(MIST_RELAY, LOW);
  
  // RGB Setup
  ledcSetup(redChannel, pwmFreq, pwmResolution);
  ledcSetup(greenChannel, pwmFreq, pwmResolution);
  ledcSetup(blueChannel, pwmFreq, pwmResolution);
  ledcAttachPin(RGB_RED, redChannel);
  ledcAttachPin(RGB_GREEN, greenChannel);
  ledcAttachPin(RGB_BLUE, blueChannel);
  controlRGB(0, 0, 255); // Initial green

  // Display
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();

  // RTC
  if (!rtc.begin()) {
    disp("RTC not found, using software clock");
    rtcSoftware.setTime(0);
  }
}

// WiFi Connection
void connectWiFi() {
  WiFi.begin(ssid, password);
  // display.clearDisplay();
  // display.setCursor(0,0);
  // display.print("Connecting WiFi");
  controlRGB(0,0,255);
  disp("Connecting WIFI");
  
  for(int i=0; i<20; i++) {
    if(WiFi.status() == WL_CONNECTED) break;
    delay(500);
    display.print(".");
    display.display();
  }
  
  internetAvailable = (WiFi.status() == WL_CONNECTED);
  // display.clearDisplay();
  // display.println(internetAvailable ? "WiFi Connected" : "Offline Mode");
  // display.display();
  disp(internetAvailable ? "WiFi Connected" : "Offline Mode");
  controlRGB(0,255,0);
  delay(1000);
  disp(String(WiFi.localIP()).c_str());
  delay(2000);
}

// Web Server Setup
void initializeWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", webpageHTML);
  });

  server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("pump", true)) {
      digitalWrite(PUMP_RELAY, !digitalRead(PUMP_RELAY));
    }
    // if(request->hasParam("mist", true)) {
    //   digitalWrite(MIST_RELAY, !digitalRead(MIST_RELAY));
    // }
    request->send(200);
  });

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
}

// Sensor Initialization
void initializeSensors() {
  if (!bmp.begin()) {
    disp("BMP180 not found!");
    while(1);
  }
  dht.begin();
}

// Audio Initialization
void initializeAudio() {
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  if (!dfPlayer.begin(Serial2)) {
    disp("DFPlayer not found!");
    while(1);
  }
  dfPlayer.volume(30);
}

// Time Management
void updateTime() {
  if (rtc.begin()) {
    state.currentTime = rtc.now();
  // } else {
  //   state.currentTime = DateTime(rtcSoftware.getEpoch());
  // }
}
}

// Sensor Reading
void readSensors() {
  static unsigned long lastRead = 0;
  if(millis() - lastRead < 2000) return;

  // BMP180
  state.temp = bmp.readTemperature();
  state.pressure = bmp.readPressure() / 100.0;
 

  // DHT11
  float h = dht.readHumidity();
  if(!isnan(h)) state.humidity = h;

 
  state.soilMoisture = map(analogRead(SOIL_MOISTURE_PIN), 4095, 0, 0, 100);
  state.lightLevel = map(analogRead(LIGHT_SENSOR_PIN), 0, 4095, 0, 100);
  state.waterLevel = map(analogRead(WATER_LEVEL_PIN), 0, 4095, 0, 100);
  lastRead = millis();
}

// Alert Handling
void handleAlerts() {
  // Soil Moisture Alert
  
  
  if(state.soilMoisture < 30 && !state.pumpActive && state.pumpInt && state.buzy) {
    triggerAlert(255,0,0,2);
    digitalWrite(PUMP_RELAY, HIGH);
    state.pumpActive = true;
    state.pumpStart = millis();
  }

  // Water Level Alert
  if(state.waterLevel < 20 && state.waterInt && state.buzy ) {
    triggerAlert(0, 0, 255, 4);
    state.waterInt = false;
    state.waterStart = millis();
  }
  if(!state.waterInt && millis() - state.waterStart > 10000){
    state.waterInt = true;
  }
  if(state.extTemp >35 && state.weatherInt && state.buzy){
    triggerAlert(255,0,0,6);
    state.weatherInt = false;
  }
  if(state.extTemp <36 && !state.weatherInt){
    state.weatherInt = true;
  }
  if(state.reportInt){
    if (state.weatherDesc == "clear sky" || state.weatherDesc == "sunny" || state.weatherDesc == "fair"){
      triggerAlert(255,255,0,7);
    }
    if(state.weatherDesc == "few clouds" || state.weatherDesc == "scattered clouds" || state.weatherDesc == "broken clouds" || state.weatherDesc == "overcast" || state.weatherDesc == "partly cloudy"){
      triggerAlert(255,255,255,8);
    }
    if(state.weatherDesc == "light rain" || state.weatherDesc == "moderate rain" || state.weatherDesc == "heavy rain"|| state.weatherDesc == "rain shower"|| state.weatherDesc == "shower rain"){
      triggerAlert(0,20,255,9);
    }
    state.report = state.weatherDesc;
    state.reportInt = false;

  }
  if(state.weatherDesc != state.report){
    state.reportInt = true;
  }
  // Humidity Alert
  // if(state.humidity < 40 && !state.mistActive) {
  //   digitalWrite(MIST_RELAY, HIGH);
  //   state.mistActive = true;
  //   triggerAlert(255, 0, 100, 5);
  //   // state.mistStart = millis();
  //   delay(2000);
  //   digitalWrite(MIST_RELAY, LOW);
  //   state.mistActive = false;
  // }

  //  Relay Timeouts
    if(state.pumpActive && millis() - state.pumpStart > 1000) {
      digitalWrite(PUMP_RELAY, LOW);
      state.pumpActive = false;
      state.pumpInt = false;
    }
    if(!state.pumpInt && millis() - state.pumpStart > 20000 ){
      state.pumpInt = true;
      digitalWrite(PUMP_RELAY, LOW);
    }
//   if(state.mistActive && millis() - state.mistStart > 2000) {
//     digitalWrite(MIST_RELAY, LOW);
//     state.mistActive = false;
//   }
}

// Scheduled Events
void handleScheduledEvents() {
  // Morning Greeting (5-9 AM)
  if(state.currentTime.hour() >=5 && state.currentTime.hour() <9 && state.lightLevel > 70 && state.buzy && greetingmode == 'D') {
    triggerAlert(255, 255, 255, 3);
    greetingmode = 'N';
  }

  // Night Greeting (10 PM-12 AM)
  if(state.currentTime.hour() >=22 && state.lightLevel < 30 && state.buzy && greetingmode == 'N') {
    triggerAlert(50, 50, 50, 1);
    greetingmode = 'D';
  }

  // Hourly Beep
  if(state.currentTime.minute() == 0 && state.currentTime.second() < 2) {
    tone(BUZZER_PIN, 1000, 500);
  }
}

// // AI Processing
// void handleAIProcessing() {
//   static unsigned long lastAICall = 0;
//   if(internetAvailable && millis() - lastAICall > 10000) {
//     getWeatherData();
//     state.aiSummary = queryGemini("Generate plant status summary with: Temp=" + 
//       String(state.temp) + "C, Humidity=" + String(state.humidity) + 
//       "%, Soil=" + String(state.soilMoisture) + ", Light=" + 
//       String(state.lightLevel) + "%");
//     lastAICall = millis();
//   }
// }

// Display Update
void updateDisplay() {
  static unsigned long lastChange = 0;
  static uint8_t screen = 0;
  
  if(millis() - lastChange > 5000) {
    screen = (screen + 1) % 11;
    lastChange = millis();
  }

  // display.clearDisplay();
  // display.setCursor(0,0);
    
    // case 0:
    //   display.printf("Time: %02d:%02d\nDate: %02d/%02d/%04d",
    //                state.currentTime.hour(), state.currentTime.minute(),
    //                state.currentTime.day(), state.currentTime.month(),
    //                state.currentTime.year());
    //   break;
    // case 1:
    //   display.printf("Temperature: %.1fC\nHumidity: %.1f%%",
    //                state.temp, state.humidity);
    //   break;
    // case 2:
    //   display.printf("Soil: %d\nWater: %d",
    //                state.soilMoisture, state.waterLevel);
    //   break;
    // case 3:
    //   display.printf("Light: %d%%\nPressure: %.1fhPa",
    //                state.lightLevel, state.pressure);
    //   break;
    // case 4:
    // display.printf("Weather: %s",state.weatherDesc.c_str()),             
    // delay(2000);
    // display.printf("AI: %s",state.aiSummary.c_str());
    //   break;
  switch(screen) {
    case 0:
      state.slide = String(state.currentTime.hour())+":"+String(state.currentTime.minute())+":"+String(state.currentTime.second());
      disp(state.slide.c_str());
      break;
    case 1:
      state.slide = String(state.currentTime.day())+"-"+String(state.currentTime.month())+ "-"+String(state.currentTime.year());
      disp(state.slide.c_str());
      break;
    case 2:
      state.slide = "Tmp: "+String(state.temp)+" C";
      disp(state.slide.c_str());
      break;
    case 3:
      state.slide = "Hum: "+String(state.humidity)+" %";
      disp(state.slide.c_str());
      break;
    case 4:
      state.slide = "Soil: "+ String(state.soilMoisture)+ " %";
      disp(state.slide.c_str());
      break;
    case 5:
      state.slide = "Water: "+ String(state.waterLevel)+ " %";
      disp(state.slide.c_str());
      break;
    case 6:
      state.slide = String(state.currentTime.hour())+":"+String(state.currentTime.minute())+":"+String(state.currentTime.second());
      disp(state.slide.c_str());
      break;
    case 7:
      state.slide = String(state.pressure)+ " hPa";
      disp(state.slide.c_str());
      break;
    case 8:
      state.slide = "LDR: "+String(state.lightLevel)+ " %";
      disp(state.slide.c_str());
      break;
    case 9:
      state.slide = String(state.weatherDesc.c_str());
      disp(state.slide.c_str());
      break;
    case 10:
      state.slide = String(state.aiSummary.c_str());
      disp(state.slide.c_str());
      break;

  }
  // display.display();
}

// Manual Trigger
void handleManualTrigger() {
  static bool lastState = false;
  bool currentState = digitalRead(TOUCH_PIN) == HIGH;
  
  if(currentState && !lastState && state.buzy) {
    static uint8_t demoMode = -1;
    demoMode = (demoMode + 1) % 9;
    
    switch(demoMode) {
      case 0:
        triggerAlert(200, 255, 200, 3); // good morning
        break;
      case 1:
        triggerAlert(10, 10, 10, 1); // good night
        break;
      case 2:
        triggerAlert(100, 20, 200, 2); // soil moisture
        break;
      case 3:
        triggerAlert(0, 10, 255, 4); // wter
        break;
      case 4:
        triggerAlert(255, 10, 20, 5); // temp high
        break;
      case 5:
        triggerAlert(255, 10, 0, 6); // wheaather temp
        break;
      case 6:
        triggerAlert(255, 255, 0, 7); // sunny
        break;
      case 7:
        triggerAlert(255, 255, 255, 8); // cloudy
        break;
      case 8:
        triggerAlert(0, 10, 255, 9); // rain
        break;
    }
  }
  lastState = currentState;
}

// WebSocket Communication
void sendDataToClients() {
  JsonDocument doc;
  doc["temp"] = state.temp;
  doc["humidity"] = state.humidity;
  doc["soil"] = state.soilMoisture;
  doc["water"] = state.waterLevel;
  doc["light"] = state.lightLevel;
  doc["weather"] = state.weatherDesc;
  doc["pressure"] = state.pressure;
  doc["aisummary"] = state.aiSummary;
  doc["extmp"] = state.extTemp;
  doc["icon"] = state.weatherIcon;
  

  String output;
  serializeJson(doc, output);
  ws.textAll(output);
}

// Alert System
void triggerAlert(uint8_t r, uint8_t g, uint8_t b, int sound) {
  if(!state.isPlaying) {
    controlRGB(r, g, b);
    tone(BUZZER_PIN, 2000, 1000);
    if(sound != 0){
      dfPlayer.stop();
      dfPlayer.play(sound);
      state.isPlaying = true;
      state.soundEnd = millis() + 2000;
      }
    if(state.isPlaying && millis() - state.soundEnd > 4000){
      state.isPlaying = false;
    }
  }
}

// RGB Control
void controlRGB(uint8_t r, uint8_t g, uint8_t b) {
  ledcWrite(redChannel, r);
  ledcWrite(greenChannel, g);
  ledcWrite(blueChannel, b);
}

// Weather API
void getWeatherData() {
  if(!internetAvailable) return;

  HTTPClient http;
  http.begin("http://api.openweathermap.org/data/2.5/weather?q=pallavaram&units=metric&appid=" + openWeatherKey);
  
  if(http.GET() == HTTP_CODE_OK) {
    JsonDocument doc;
    deserializeJson(doc, http.getString());
    state.weatherDesc = doc["weather"][0]["description"].as<String>();
    state.extTemp = doc["main"]["temp"];
    state.weatherIcon = doc["weather"][0]["icon"].as<String>();
  }
  http.end();
}


// Replace queryGemini function with this Groq implementation
String queryGroq(String prompt) {
  if(!internetAvailable) return "Offline";
  
  HTTPClient http;
  http.begin("https://api.groq.com/openai/v1/chat/completions");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + Key);
  // Create a friendly system message
  String systemMessage = "You are Smart Plant V1.0, a friendly AI responsive for plant care. ";
  systemMessage += "Always respond in a direct and related and short";
  systemMessage += "your are actually a Cactus plant.";
  systemMessage += "Created by Naveen, Anuub, and Bharath from Vels University. ";
  systemMessage += "Keep responses concise (under 120 characters). ";
  systemMessage += "Your role: provide direct answers to plant care questions.";
  systemMessage += "Use simple language and context focused. ";
  systemMessage += "Rules: 1. Only answer the user's question directly 2. Never ask follow-up questions 3. Keep responses under 15 words 4. Use simple language dont give irrelevent";

  String payload = "{";
  payload += "\"model\": \"" + model + "\",";
  payload += "\"messages\": [";
  payload += "{\"role\": \"system\", \"content\": \"" + systemMessage + "\"},";
  payload += "{\"role\": \"user\", \"content\": \"" + prompt + "\"}";
  payload += "],";
  payload += "\"temperature\": 1.3,"; // Makes responses more friendly/creative
  payload += "\"presence_penalty\": 0.5,";
  payload += "\"max_tokens\": 120"; // Limit response length
  payload += "}";

  int httpCode = http.POST(payload);
  if(httpCode == HTTP_CODE_OK) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, http.getString());
    if(error) {
      Serial.print("JSON Error: ");
      Serial.println(error.c_str());
      return "API Error";
    }
    return doc["choices"][0]["message"]["content"].as<String>();
  }
  else {
    Serial.print("AI API Error: ");
    Serial.println(httpCode);
    return "API Error: " + String(httpCode);
  }
  http.end();
  return "Error";
}

// In handleAIProcessing() replace Gemini call with Groq
void handleAIProcessing() {
  static unsigned long lastAICall = 0;
  if(internetAvailable && (millis() - lastAICall > 60000 || millis() < 60000)) {
    getWeatherData();
    String prompt = "Generate a concise plant status summary using this data: ";
    prompt += "Temperature: " + String(state.temp) + "°C, ";
    prompt += "Humidity: " + String(state.humidity) + "%, ";
    prompt += "Soil Moisture : " + String(state.soilMoisture) + ", ";
    prompt += "Light Level: " + String(state.lightLevel) + "%, ";
    prompt += "Air pressure: "+ String(state.pressure)+"hPa,";
    prompt += "open weather: " + String(state.weatherDesc) + ", ";
    prompt += "open weather temperature: " + String(state.extTemp) + "°C.";
    prompt += "Keep response under 120 characters and simple and context.";

    state.aiSummary = queryGroq(prompt) ;
    lastAICall = millis();
  }
}
// WebSocket Events
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
  AwsEventType type, void *arg, uint8_t *data, size_t len) {
if(type == WS_EVT_CONNECT) {
disp("WebSocket client connected");
sendDataToClients();
}
else if(type == WS_EVT_DATA) {
AwsFrameInfo *info = (AwsFrameInfo*)arg;
if(info->final && info->index == 0) {
data[len] = 0;
JsonDocument doc;
DeserializationError error = deserializeJson(doc, data);
if(!error && doc["type"] == "chat") {

String prompt = "User: " + doc["message"].as<String>();

prompt += "  Current sensor data: ";
prompt += "Temp: " + String(state.temp) + "°C, " ;
prompt += "Humidity: " + String(state.humidity) + "%, " ;
prompt += "Soil: " + String(state.soilMoisture) + "%, " ;
prompt += "Light: " + String(state.lightLevel) + "%," ;
prompt += "Air pressure: "+ String(state.pressure)+"hPa,";
prompt += "open weather: "+ String(state.weatherDesc)+ ", ";
prompt += "open weather temp:" + String(state.extTemp)+"°C.";

String response = queryGroq(prompt);

JsonDocument respDoc;
respDoc["aichat"] = response;
String output;
serializeJson(respDoc, output);
ws.textAll(output);
}
}
}
}