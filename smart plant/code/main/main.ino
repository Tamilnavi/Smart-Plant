#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>
#include <DFRobotDFPlayerMini.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>

// OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// BMP180 Sensor
Adafruit_BMP085 bmp;

// DHT11 Sensor
#define DHTPIN 14
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Capacitive Soil Moisture Sensor
#define SOIL_MOISTURE_PIN 34

// Water Level Sensor
#define WATER_LEVEL_PIN 35

// Relay
#define RELAY_PIN 23

// Buzzer
#define BUZZER_PIN 5

// RGB LED
#define LED_R 25
#define LED_G 26
#define LED_B 27

// Touch Sensor
#define TOUCH_PIN 13

// DF Mini Player
// #define DF_RX 16
// #define DF_TX 17
// HardwareSerial mySerial(DF_RX, DF_TX);
// DFRobotDFPlayerMini dfPlayer;

// RTC
RTC_DS3231 rtc;

// Web Server
AsyncWebServer server(80);

void setup() {
    Serial.begin(115200);
    Wire.begin();
    
    // Initialize OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
    }
    display.clearDisplay();
    display.display();
    
    // Initialize BMP180
    if (!bmp.begin()) {
        Serial.println("Could not find BMP180 sensor! Check wiring.");
    }
    
    // Initialize DHT
    dht.begin();
    
    // // Initialize DFPlayer Mini
    // mySerial.begin(9600);
    // if (!dfPlayer.begin(mySerial)) {
    //     Serial.println("DFPlayer Mini not detected!");
    // }
    // dfPlayer.volume(20);
    
    // Initialize RTC
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
    }
    
    // Set pin modes
    pinMode(SOIL_MOISTURE_PIN, INPUT);
    pinMode(WATER_LEVEL_PIN, INPUT);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    pinMode(TOUCH_PIN, INPUT);
    
    // Start Web Server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "ESP32 IoT System Running!");
    });
    server.begin();
}

void loop() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    float pressure = bmp.readPressure() / 100.0;
    int soilMoisture = analogRead(SOIL_MOISTURE_PIN);
    int waterLevel = analogRead(WATER_LEVEL_PIN);
    DateTime now = rtc.now();
    
    // Display on OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Temp: "); display.print(temperature); display.println(" C");
    display.print("Humidity: "); display.print(humidity); display.println("%");
    display.print("Pressure: "); display.print(pressure); display.println(" hPa");
    display.print("Soil: "); display.print(soilMoisture);
    display.print("  Water: "); display.println(waterLevel);
    display.print("Time: "); display.print(now.hour());
    display.print(":"); display.print(now.minute());
    display.print(":"); display.println(now.second());
    display.display();
    
    // Control Relay based on Soil Moisture
    if (soilMoisture < 500) {
        digitalWrite(RELAY_PIN, HIGH);
        // dfPlayer.play(1); // Play sound "Watering plant"
    } else {
        digitalWrite(RELAY_PIN, LOW);
    }
    
    // Touch Sensor controls Buzzer
    if (digitalRead(TOUCH_PIN) == HIGH) {
        tone(BUZZER_PIN, 1000);
        delay(500);
        noTone(BUZZER_PIN);
    }
    
    delay(2000);
}
