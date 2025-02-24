#include <math.h>
#include <DHT.h>
#define _SS_MAX_TX_BUFF 256
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

#define DHTPIN 5
#define DHTTYPE DHT11
#define ESP_RX 10  // Pin for SoftwareSerial RX on Arduino
#define ESP_TX 11  // Pin for SoftwareSerial TX on Arduino

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial espSerial(ESP_RX, ESP_TX);  // RX, TX

static const byte bucketPin = 2;
volatile int bucketTipped;

static const byte anemometerPin = 3;
volatile int anemometerSpin;

static const int interval = 5000;
unsigned long lastMillis = 0;

float windSpeed() {
  float pulses = anemometerSpin;                 // Number of pulses in interval
  anemometerSpin = 0;                            // Reset count
  float seconds = interval / 1000.0;             // Convert interval to seconds
  float pulsesPerSecond = pulses / seconds;      // Pulse frequency
  float windSpeedMPH = pulsesPerSecond * 1.492;  // 80422 spec: 1 pulse/sec = 1.492 MPH
  float windSpeedKPH = windSpeedMPH * 1.60934;   // Convert to km/h
  return round(windSpeedKPH * 100.0) / 100.0;    // Return km/h, rounded to 2 decimals
}

float readWindDirection() {
  const float vaneReadingInVolts = analogRead(A0) * 5.0 / 1024.0;
  const float headingVolt[16] = { 3.84, 1.98, 2.25, 0.41, 0.45, 0.32, 0.90, 0.62, 1.4, 1.19, 3.08, 2.93, 4.62, 4.04, 4.33, 3.43 };
  const float threshold = 0.05;

  for (int x = 0; x < 16; x++) {
    if (abs(vaneReadingInVolts - headingVolt[x]) < threshold) {
      return x * 22.5;  // Simplified: each step is 22.5 degrees
    }
  }
  return 0;  // Default to North if no match
}

float rainFall() {
  const float bucketSize = 0.2794;  // mm per tip
  float rainFall = bucketTipped * bucketSize;
  bucketTipped = 0;
  return rainFall;
}

void setupStation() {
  bucketTipped = 0;
  anemometerSpin = 0;
  pinMode(bucketPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(bucketPin), bucketTipCounter, FALLING);
  pinMode(anemometerPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(anemometerPin), anemometerSpinCounter, RISING);
}

void bucketTipCounter() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 50) {
    bucketTipped++;
  }
  last_interrupt_time = interrupt_time;
}

void anemometerSpinCounter() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 50) {  // Add debounce
    anemometerSpin++;
  }
  last_interrupt_time = interrupt_time;
}

void setup() {
  Serial.begin(115200);   // Debugging to PC
  espSerial.begin(9600);  // Match ESP-01S baud rate
  dht.begin();
  pinMode(A3, INPUT);
  setupStation();
}

const char* readWaterLevel() {
  int waterLevelValue = analogRead(A1);
  if (waterLevelValue == 0) return "Empty";
  if (waterLevelValue <= 420) return "Low";
  if (waterLevelValue <= 520) return "Medium";
  return "High";
}

const char* readPhotoResistor() {
  int photoResistorValue = analogRead(A2);
  if (photoResistorValue < 10) return "Very bright";
  if (photoResistorValue < 200) return "Bright";
  if (photoResistorValue < 500) return "Light";
  if (photoResistorValue < 800) return "Dim";
  return "Dark";
}

float readVolt() {
  constexpr float R1 = 2192.0;
  constexpr float R2 = 988.5;
  const float value = analogRead(A3);
  const float vOut = (value * 4.2) / 1024.0;
  const float vIn = vOut / (R2 / (R1 + R2));
  return round(vIn * 100.0) / 100.0;
}

void loop() {
  if (millis() - lastMillis >= interval) {
    lastMillis = millis();

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    float hic = dht.computeHeatIndex(t, h, false);

    StaticJsonDocument<256> doc;  // Increased size for more fields
    doc["temperature"] = t;       // Match ESP-01S keys
    doc["humidity"] = h;
    doc["heatIndex"] = round(hic * 100.0) / 100.0;
    doc["windDirection"] = readWindDirection();
    doc["windSpeed"] = windSpeed();
    doc["rainFall"] = rainFall();
    doc["light"] = readPhotoResistor();
    doc["waterLevel"] = readWaterLevel();
    doc["voltage"] = readVolt();

    String json;
    serializeJson(doc, json);
    Serial.println(json);     // Debug to PC
    espSerial.println(json);  // Send to ESP-01S
  }
}