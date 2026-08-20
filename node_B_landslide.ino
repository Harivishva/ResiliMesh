// ============================================================
// NODE B - Hilly / Landslide Zone
// Sensors: Rain + Soil moisture + Vibration
// Relays Node A's data (received via LoRa) combined with its
// own readings onward to Node C every 5 seconds
// ============================================================

#include <SPI.h>
#include <LoRa.h>

// LoRa pins
#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 2

// Sensor pins
#define SOIL_PIN       34   // analog
#define RAIN_PIN       35   // analog
#define VIBRATION_PIN  33   // digital

const long SEND_INTERVAL = 5000;
unsigned long lastSend = 0;

String lastAData = "";
unsigned long lastAReceivedTime = 0;
const unsigned long A_DATA_TIMEOUT = 30000; // treat A's data as stale after 30s

// ---- Calibrate these against your actual sensors ----
int SOIL_AIR_VALUE   = 4095;  // raw value when soil sensor is dry/in air
int SOIL_WATER_VALUE = 1200;  // raw value when soil sensor is in water
int RAIN_DRY_VALUE   = 4095;  // raw value when rain sensor is dry
int RAIN_WET_VALUE   = 1500;  // raw value when rain sensor is fully wet

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(VIBRATION_PIN, INPUT);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {        // must match Node A and Node C
    Serial.println("LoRa init failed. Check wiring.");
    while (1);
  }

  Serial.println("Node B ready");
}

void loop() {
  // 1. Listen for incoming data from Node A
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }
    if (incoming.startsWith("A|")) {
      lastAData = incoming;
      lastAReceivedTime = millis();
      Serial.println("Received from A: " + incoming);
    }
  }

  // 2. Every SEND_INTERVAL, read own sensors and forward combined data to C
  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();

    int soilRaw = analogRead(SOIL_PIN);
    int rainRaw = analogRead(RAIN_PIN);
    int vibration = digitalRead(VIBRATION_PIN);

    int soilPercent = map(soilRaw, SOIL_AIR_VALUE, SOIL_WATER_VALUE, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    int rainPercent = map(rainRaw, RAIN_DRY_VALUE, RAIN_WET_VALUE, 0, 100);
    rainPercent = constrain(rainPercent, 0, 100);

    String bData = "B|rain:" + String(rainPercent) +
                    "|soil:" + String(soilPercent) +
                    "|vib:" + String(vibration);

    String combined;
    if (lastAData != "" && (millis() - lastAReceivedTime < A_DATA_TIMEOUT)) {
      combined = lastAData + ";" + bData;
    } else {
      combined = "A|status:no_data;" + bData;
    }

    LoRa.beginPacket();
    LoRa.print(combined);
    LoRa.endPacket();

    Serial.println("Relayed to C: " + combined);
  }
}
