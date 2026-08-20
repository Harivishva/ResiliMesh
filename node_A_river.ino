// ============================================================
// NODE A - River Zone
// Sensors: Ultrasonic (water level) + BMP280 (temp/pressure)
// Sends readings via LoRa every 5 seconds
// ============================================================

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

// LoRa pins
#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 2

// Ultrasonic pins
#define TRIG_PIN 27
#define ECHO_PIN 26   // MUST go through voltage divider (5V -> 3.3V)

Adafruit_BMP280 bmp;

const long SEND_INTERVAL = 5000;
unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Wire.begin(21, 22); // SDA, SCL
  if (!bmp.begin(0x76)) {          // try 0x77 if this fails
    Serial.println("BMP280 not found - check wiring/I2C address");
  }

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {        // match this frequency on ALL nodes
    Serial.println("LoRa init failed. Check wiring.");
    while (1);
  }

  Serial.println("Node A ready");
}

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout (~5m range)
  if (duration == 0) return -1; // no echo received

  return duration * 0.0343 / 2; // convert to cm
}

void loop() {
  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();

    float distance = readDistanceCm();   // distance from sensor down to water surface
    float pressure = bmp.readPressure() / 100.0F; // hPa
    float temp = bmp.readTemperature();  // Celsius

    String packet = "A|water_cm:" + String(distance, 1) +
                     "|pressure_hpa:" + String(pressure, 1) +
                     "|temp_c:" + String(temp, 1);

    LoRa.beginPacket();
    LoRa.print(packet);
    LoRa.endPacket();

    Serial.println("Sent: " + packet);
  }
}

// NOTE: "water_cm" is the distance from the sensor DOWN to the water surface.
// If the sensor is mounted at a fixed height H above the riverbed/baseline,
// actual water level = H - water_cm. Calibrate H for your deployment site.
