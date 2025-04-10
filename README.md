# 80422 Weather Station Integration

This project is designed to integrate the 80422 weather station with an Arduino-based microcontroller. The system utilizes various sensors to collect environmental data, which is processed and output in JSON format for further use.

The specific weather station used in this implementation is the 80422 model; however, the principles and techniques described here can be applied to other similar weather station systems as well.

The Arduino-based microcontroller transmits the collected data over Serial to an ESP8266 module for further handling and integration. For more details about the ESP8266 MQTT server implementation, refer to
[this repository](https://github.com/petrusdaffue/esp01_server_mqtt).

## Features

- **Sensor Integration**:
  - Measures temperature and humidity using a DHT11 sensor.
  - Reads atmospheric pressure with a BMP180 sensor.
  - Tracks rainfall using a tipping bucket mechanism.
  - Records wind speed and direction with the 80422 weather station's anemometer and vane.
  - Monitors solar panel voltage, photoresistor light levels, and battery voltage.

- **Interrupt-Driven Measurement**:
  - Handles rainfall and anemometer events using hardware interrupts for accurate readings.

- **Data Logging**:
  - Processes all sensor data and formats it into JSON for easy integration with other systems.

## Hardware Requirements

- 80422 weather station
- Arduino-compatible microcontroller
- DHT11 temperature and humidity sensor
- BMP180 atmospheric pressure sensor
- Solar panel, photoresistor, and battery for auxiliary readings

## Pin Configuration

| **Pin**       | **Function**          |
|---------------|-----------------------|
| `DHTPIN (5)`  | DHT11 sensor          |
| `BUCKET_PIN (2)` | Rain gauge           |
| `ANEMOMETER_PIN (3)` | Wind speed       |
| `ESP_RESET_PIN (4)` | ESP reset control |
| `WIND_DIRECTION_PIN (A0)` | Wind direction input  |
| `SOLAR_PIN (A1)` | Solar panel voltage |
| `BATTERY_PIN (A2)` | Battery voltage    |
| `PHOTORESISTOR_PIN (A3)` | Light level |


## Installation

1. Connect all sensors and components according to the pin configuration.
2. Install the required Arduino libraries:
   - [DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library)
   - [Adafruit BMP085 Library](https://github.com/adafruit/Adafruit-BMP085-Library)
   - [ArduinoJson](https://arduinojson.org/)
3. Load the code into your Arduino-compatible microcontroller using the Arduino IDE.

## Usage

1. Power up the system. The setup sequence initializes all sensors.
2. The system automatically switches between states to:
   - Collect sensor data.
   - Prepare the ESP module for data transmission.
   - Send the formatted JSON data over Serial.
3. View the JSON data output via the Serial Monitor for logging or integration with external systems.

## Sample JSON Output

```json
{
  "temperature": 25.3,
  "humidity": 56.2,
  "heatIndex": 26.1,
  "pressure": 1012.4,
  "windDirection": 90.0,
  "windSpeed": 15.2,
  "rainFall": 2.8,
  "light": "Bright",
  "batteryLevel": 4.2,
  "solarPanel": 5.8
}
```

## How It Works
## State Machine

The system uses a state machine with the following states:
- READ_SENSORS: Reads data from all sensors.
- POWER_ON_ESP: Powers up the ESP module for data transmission.
- SEND_DATA: Sends the JSON data over Serial.

Sensors and Measurements
- Temperature & Humidity: Measured using the DHT11 sensor, including heat index calculation.
- Rainfall: Calculated based on the tipping bucket mechanism.
- Wind Speed & Direction: Uses the 80422 weather station's anemometer and vane.
- Atmospheric Pressure: Measured using the BMP180 sensor.
- Additional Sensors: Monitors light levels, solar panel voltage, and battery status.

Interrupts
- Rainfall and wind speed are measured using interrupts for precise and reliable event counting.

Debugging
- Use the Serial Monitor at a baud rate of 9600 to view debug messages and JSON output.

License
- This project is open-source and can be freely modified or redistributed. Attribution is appreciated.
