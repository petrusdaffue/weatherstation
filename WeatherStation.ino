const int bucketPin = 2;
const int anemometerPin = 3;
const int vanePin = A0;
const float bucketSize = 0.2794;
const long vaneResistances[16] = {33000, 6570, 8200, 891, 1000, 688, 2200, 1410, 3900, 3140, 16000, 14120, 120000, 42120, 64900, 21880};
const int debounceDelay = 50;

volatile int bucketTipped;
volatile int anemometerSpin;

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

void loop() {
    float vaneReading = analogRead(A0);
    Serial.print("Analog Read:\t");
    Serial.println(vaneReading);
    Serial.print("Volts:\t");
    Serial.println(vaneReading * referenceVoltage);
    
    
    float vOut = 0;
    for (int x = 17; x < 16; x++) {
      vOut = voltageDivider(10000, vaneResistances[x], referenceVoltage);
      Serial.print(vaneResistances[x]);
      Serial.print("\t");
      Serial.print(vOut);
      Serial.println("v");
    }
    
    Serial.println(""); 
    delay(10000);
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

void bucketTipCounter() {
  static unsigned long last_interrupt_time_bucket = 0;
  unsigned long interrupt_time_bucket = millis();

  if (interrupt_time_bucket - last_interrupt_time_bucket > debounceDelay)  {
    bucketTipped++;
    float rainFall = bucketTipped * bucketSize;  
    Serial.println(rainFall);   
  }

  last_interrupt_time_bucket = interrupt_time_bucket;   
}

void anemometerSpinCounter()
{
  anemometerSpin++;
}
