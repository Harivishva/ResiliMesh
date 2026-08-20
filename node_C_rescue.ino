// ============================================================
// NODE C - Rescue Department
// Receives combined data from Node B via LoRa, computes risk
// status, and hosts a LOCAL webpage over its own WiFi hotspot
// (no internet or router required - fully offline)
// ============================================================

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>

// LoRa pins
#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 2

// ---- WiFi Access Point settings ----
const char* ap_ssid     = "DisasterMeshNode-C";
const char* ap_password = "rescue123";   // must be 8+ characters

WebServer server(80);

// ---- Latest sensor data ----
float waterCm = -1, pressureHpa = -1, tempC = -1;
int rainPercent = -1, soilPercent = -1, vibration = -1;
bool aDataAvailable = false;
unsigned long lastPacketTime = 0;

String floodRisk = "No data";
String landslideRisk = "No data";

// ---- Helper: extract a value between "key:" and next "|" ----
String extractValue(String data, String key) {
  int start = data.indexOf(key);
  if (start < 0) return "";
  start += key.length();
  int end = data.indexOf('|', start);
  if (end < 0) end = data.length();
  return data.substring(start, end);
}

void computeRisk() {
  if (aDataAvailable) {
    if (waterCm >= 0 && waterCm < 30) floodRisk = "HIGH";
    else if (waterCm < 60) floodRisk = "MODERATE";
    else if (waterCm >= 0) floodRisk = "NORMAL";
    else floodRisk = "Sensor error";
  } else {
    floodRisk = "No data from Node A";
  }

  if (soilPercent >= 0) {
    int score = 0;
    if (soilPercent > 70) score++;
    if (rainPercent > 60) score++;
    if (vibration == 1) score++;

    if (score >= 2) landslideRisk = "HIGH";
    else if (score == 1) landslideRisk = "MODERATE";
    else landslideRisk = "NORMAL";
  } else {
    landslideRisk = "No data from Node B";
  }
}

void parsePacket(String data) {
  int sep = data.indexOf(';');
  String aPart = sep >= 0 ? data.substring(0, sep) : data;
  String bPart = sep >= 0 ? data.substring(sep + 1) : "";

  if (aPart.indexOf("water_cm:") >= 0) {
    waterCm = extractValue(aPart, "water_cm:").toFloat();
    pressureHpa = extractValue(aPart, "pressure_hpa:").toFloat();
    tempC = extractValue(aPart, "temp_c:").toFloat();
    aDataAvailable = true;
  } else {
    aDataAvailable = false;
  }

  if (bPart.indexOf("rain:") >= 0) {
    rainPercent = extractValue(bPart, "rain:").toInt();
    soilPercent = extractValue(bPart, "soil:").toInt();
    vibration = extractValue(bPart, "vib:").toInt();
  }

  computeRisk();
}

String riskClass(String risk) {
  if (risk.indexOf("HIGH") >= 0) return "high";
  if (risk.indexOf("MODERATE") >= 0) return "moderate";
  return "normal";
}

// ---- Local webview served by Node C ----
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='5'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Disaster Monitoring Dashboard</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:20px;background:#f4f4f4;color:#222}";
  html += "h1{color:#2c3e50}";
  html += ".card{background:white;padding:15px 20px;margin:14px 0;border-radius:8px;box-shadow:0 1px 4px rgba(0,0,0,0.15)}";
  html += ".high{color:#c0392b;font-weight:bold}";
  html += ".moderate{color:#e67e22;font-weight:bold}";
  html += ".normal{color:#27ae60;font-weight:bold}";
  html += "</style></head><body>";

  html += "<h1>Disaster Monitoring Dashboard</h1>";

  html += "<div class='card'><h2>Node A - River Zone</h2>";
  html += "<p>Water level distance: " + String(waterCm) + " cm</p>";
  html += "<p>Pressure: " + String(pressureHpa) + " hPa</p>";
  html += "<p>Temperature: " + String(tempC) + " &deg;C</p>";
  html += "<p>Flood risk: <span class='" + riskClass(floodRisk) + "'>" + floodRisk + "</span></p>";
  html += "</div>";

  html += "<div class='card'><h2>Node B - Hilly Zone</h2>";
  html += "<p>Rain: " + String(rainPercent) + " %</p>";
  html += "<p>Soil moisture: " + String(soilPercent) + " %</p>";
  html += "<p>Vibration detected: " + String(vibration == 1 ? "YES" : "NO") + "</p>";
  html += "<p>Landslide risk: <span class='" + riskClass(landslideRisk) + "'>" + landslideRisk + "</span></p>";
  html += "</div>";

  unsigned long secsAgo = lastPacketTime == 0 ? 0 : (millis() - lastPacketTime) / 1000;
  html += "<p style='color:#888'>Last update: " + String(secsAgo) + "s ago</p>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {        // must match Node A and Node B
    Serial.println("LoRa init failed. Check wiring.");
    while (1);
  }

  // Create local WiFi hotspot - rescue team connects directly, no internet needed
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("Access Point started. Connect to WiFi: ");
  Serial.println(ap_ssid);
  Serial.print("Then open in browser: http://");
  Serial.println(WiFi.softAPIP()); // usually 192.168.4.1

  server.on("/", handleRoot);
  server.begin();

  Serial.println("Node C ready - dashboard live");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }
    Serial.println("Received: " + incoming);
    lastPacketTime = millis();
    parsePacket(incoming);
  }

  server.handleClient();
}
