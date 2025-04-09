#include <math.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>

// Pin Definitions
#define DHTPIN 5                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
#define DHTTYPE DHT11
#define ESP_RESET_PIN 4
#define BUCKET_PIN 2
#define ANEMOMETER_PIN 3
#define WIND_DIRECTION_PIN A0
#define SOLAR_PIN A1
#define BATTERY_PIN A2
#define PHOTORESISTOR_PIN A3


// Sensor Instances
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;

// Constants
const unsigned long INTERVAL = 60000;  // 60 seconds
const float BUCKET_SIZE = 0.2794;      // mm per tip

// Global Variables
volatile int bucketTipped = 0;
volatile unsigned long anemometerSpin = 0;
unsigned long lastMillis = 0;
unsigned long currentMillis = 0;
unsigned long stateStartTime = 0;
String json;
bool isFirstRun = true;
bool doSend = false;

// State Machine
enum State { READ_SENSORS,
             POWER_ON_ESP,
             SEND_DATA };
State currentState = READ_SENSORS;

// Interrupt Handlers
void bucketTipCounter() {
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastTime > 50) {  // Debounce 50ms
    bucketTipped++;
    lastTime = currentTime;
  }
}

void anemometerSpinCounter() {
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastTime > 50) {  // Debounce 50ms
    anemometerSpin++;
    lastTime = currentTime;
  }
}

// Sensor Functions
float windSpeed() {
  float pulses = anemometerSpin;
  anemometerSpin = 0;
  float seconds = INTERVAL / 1000.0;
  if (seconds <= 0) return 0.0;
  float pulsesPerSecond = pulses / seconds;
  float windSpeedKPH = (pulsesPerSecond * 1.492) * 1.60934;  // MPH to KPH
  return round(windSpeedKPH * 100.0) / 100.0;                // 2 decimals
}

float readWindDirection() {
  float vaneVolts = analogRead(WIND_DIRECTION_PIN) * 5.0 / 1024.0;
  const float headingVolt[] = { 3.84, 1.98, 2.25, 0.41, 0.45, 0.32, 0.90, 0.62,
                                1.40, 1.19, 3.08, 2.93, 4.62, 4.04, 4.33, 3.43 };
  const float threshold = 0.05;
  for (int i = 0; i < 16; i++) {
    if (abs(vaneVolts - headingVolt[i]) < threshold) {
      return i * 22.5;  // Degrees
    }
  }
  return 0.0;  // Default if no match
}

float rainFall() {
  float rain = bucketTipped * BUCKET_SIZE;
  bucketTipped = 0;
  return rain;
}

float readSolarPanel() {
  float voltage = analogRead(SOLAR_PIN) * (5.0 / 1023.0);
  return voltage * 1.5;
}

float readPressure() {
  float pressure = bmp.readPressure() / 100.0;  // Pa to hPa
  return isnan(pressure) ? -999 : round(pressure * 10.0) / 10.0;
}

const char* readPhotoResistor() {
  int value = analogRead(PHOTORESISTOR_PIN);
  if (value < 10) return "Very bright";
  if (value < 200) return "Bright";
  if (value < 500) return "Light";
  if (value < 800) return "Dim";
  return "Dark";
}

float readBatteryLevel() {
  return analogRead(BATTERY_PIN) * (5.0 / 1023.0);
}

// Setup Functions
void setupStation() {
  pinMode(BUCKET_PIN, INPUT_PULLUP);
  pinMode(ANEMOMETER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUCKET_PIN), bucketTipCounter, FALLING);
  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), anemometerSpinCounter, RISING);
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  if (!bmp.begin()) {
    Serial.println("BMP180 init failed!");
    while (1)
      ;  // Halt if sensor fails
  }

  dht.begin();
  pinMode(SOLAR_PIN, INPUT);
  pinMode(PHOTORESISTOR_PIN, INPUT);
  pinMode(BATTERY_PIN, INPUT);
  pinMode(ESP_RESET_PIN, OUTPUT);
  digitalWrite(ESP_RESET_PIN, HIGH);

  setupStation();
}

// Main Loop
void loop() {
  currentMillis = millis();

  switch (currentState) {
    case READ_SENSORS:
      if (currentMillis - lastMillis >= INTERVAL || isFirstRun) {
        float temp = dht.readTemperature();
        float humidity = dht.readHumidity();
        if (isnan(temp)) temp = -999;
        if (isnan(humidity)) humidity = -999;
        float heatIndex = dht.computeHeatIndex(temp, humidity, false);

        StaticJsonDocument<384> doc;
        doc["temperature"] = temp;
        doc["humidity"] = humidity;
        doc["heatIndex"] = round(heatIndex * 100.0) / 100.0;
        doc["pressure"] = readPressure();
        doc["windDirection"] = readWindDirection();
        doc["windSpeed"] = windSpeed();
        doc["rainFall"] = rainFall();
        doc["light"] = readPhotoResistor();
        doc["batteryLevel"] = readBatteryLevel();
        doc["solarPanel"] = readSolarPanel();

        json = "";  // Clear previous data
        serializeJson(doc, json);

        isFirstRun = false;
        lastMillis = currentMillis;

        // Wake ESP
        digitalWrite(ESP_RESET_PIN, LOW);
        delay(10);
        digitalWrite(ESP_RESET_PIN, HIGH);

        stateStartTime = currentMillis;
        currentState = POWER_ON_ESP;
        doSend = true;
      }
      break;

    case POWER_ON_ESP:
      if (currentMillis - stateStartTime >= 2500) {
        stateStartTime = currentMillis;
        currentState = SEND_DATA;
      }
      break;

    case SEND_DATA:
      if (doSend) {
        Serial.println(json);
        doSend = false;
      }
      if (currentMillis - stateStartTime >= 3000) {
        currentState = READ_SENSORS;
      }
      break;
  }
}