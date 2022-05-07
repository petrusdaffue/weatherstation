#include <math.h>

const int bucketPin = 2;
volatile int bucketTipped; 
const float bucketSize = 0.2794;
const int debounceDelay = 50;

const int vanePin = A0;


const int anemometerPin = 3;
volatile int anemometerSpin;

int interval = 5000;

float windSpeed() {
  float rotations = anemometerSpin / 2.0;
  anemometerSpin = 0;

  float radiusCm = 2.0;
  float circumferenceCm = (2 * M_PI) * radiusCm;
  float distanceCm = circumferenceCm * rotations;
  
  float windSpeedPerSecond = distanceCm / interval; 
  float windSpeedPerHour = windSpeedPerSecond * 60 * 60;
  
  return windSpeedPerHour;
}


float referenceVoltage = 5.0;
                   
void setup() {
  Serial.begin(9600);

  bucketTipped = 0;
  anemometerSpin = 0;
  
  pinMode(bucketPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(bucketPin), bucketTipCounter, FALLING);

  pinMode(anemometerPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(anemometerPin), anemometerSpinCounter, RISING);
}

String vaneDirectionFromVolts(float vaneReadingInVolts) {
  const float headingVolt[16][2] = {{ 0,  3.84 }, { 22.5, 1.98 }, { 45, 2.25 }, { 67.5, 0.41 }, 
                                  { 90, 0.45 }, { 112.5, 0.32 }, { 135, 0.90 }, { 157.5, 0.62 }, 
                                  { 180, 1.4}, { 202.5, 1.19 }, { 225, 3.08 }, { 247.5, 2.93 },
                                  { 270, 4.62 }, { 292.5, 4.04 }, { 315, 4.33 }, { 337.5, 3.43 }};
  const String heading[16] = { "N", "NNE", "NE", "ENE", 
                             "E", "ESE", "SE", "SSE", 
                             "S", "SSW", "SW", "WSW", 
                             "W", "WNW", "NW", "NNW" };

  
  for (int x = 0; x < 16; x++) {
    if (abs(vaneReadingInVolts - headingVolt[x][1]) < 0.05) {
      return heading[x];
    }
  }
}


void sendData() {
    float windSpeedPerHour = windSpeed();

    float vaneReading = analogRead(vanePin);
    float vaneReadingInVolts = vaneReading * referenceVoltage / 1024;
    String windDirection = vaneDirectionFromVolts(vaneReadingInVolts);
    
    float rainFallInMm = rainFall();
    
    Serial.println(windDirection + "\t" + String(windSpeedPerHour) + " km/h\t" + String(rainFallInMm) + " mm");
   
}

void loop() {
    sendData();
    
    delay(interval);
}

void resetRainFall() {
  bucketTipped = 0;
}

void resetAnemometerSpin() {
  anemometerSpin = 0;
}

float voltageDivider(long R1, long R2, float vIn)
{
  float vOut = vIn * R2/(R1 + R2);
  
  return vOut;
}


float rainFall() {
  float rainFall = bucketTipped * bucketSize;   
  bucketTipped = 0;
  
  return rainFall;
}


void bucketTipCounter() {
  static unsigned long last_interrupt_time_bucket = 0;
  unsigned long interrupt_time_bucket = millis();

  if (interrupt_time_bucket - last_interrupt_time_bucket > debounceDelay)  {
    bucketTipped++;
  }

  last_interrupt_time_bucket = interrupt_time_bucket;   
}


void anemometerSpinCounter()
{
  anemometerSpin++;   
}
