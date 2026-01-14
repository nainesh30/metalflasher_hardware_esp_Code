#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <ArduinoJson.h>

// ========== CONFIG ==========
#define GATEWAY_ID "room_01"
#define RSSI_THRESHOLD -95

// UUID: 6a2d91f4-0cbb-4c9e-8a6a-1b8f0c3e9a11
const uint8_t TARGET_UUID[16] = {
  0x6A,0x2D,0x91,0xF4,
  0x0C,0xBB,
  0x4C,0x9E,
  0x8A,0x6A,
  0x1B,0x8F,0x0C,0x3E,0x9A,0x11
};

// WiFi
const char* WIFI_SSID = "PPSU_STUDENT";
const char* WIFI_PASS = "Ppsu@Student_2024";

// 🔴 CHANGE THIS TO YOUR PC IP
const char* MQTT_BROKER = "172.16.4.198";
const int   MQTT_PORT   = 1883;
const char* MQTT_TOPIC  = "ble/scan";
// ============================

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
BLEScan* pBLEScan;

struct BeaconData {
  uint16_t major;
  uint16_t minor;
  int rssi;
  bool valid;
};

BeaconData beacon;
unsigned long lastScan = 0;
unsigned long lastPublish = 0;

// ---------- BLE CALLBACK ----------
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) {

    if (beacon.valid) return;
    if (!d.haveManufacturerData()) return;

    String md = d.getManufacturerData();
    if (md.length() < 25) return;

    const uint8_t* p = (const uint8_t*)md.c_str();

    // iBeacon check
    if (p[0]!=0x4C || p[1]!=0x00 || p[2]!=0x02 || p[3]!=0x15) return;
    if (memcmp(&p[4], TARGET_UUID, 16) != 0) return;

    int rssi = d.getRSSI();
    if (rssi < RSSI_THRESHOLD) return;

    beacon.major = (p[20] << 8) | p[21];
    beacon.minor = (p[22] << 8) | p[23];
    beacon.rssi  = rssi;
    beacon.valid = true;

    Serial.printf("[BLE] Major:%d Minor:%d RSSI:%d\n",
                  beacon.major, beacon.minor, beacon.rssi);
  }
};

// ---------- WIFI ----------
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WiFi] Connected");
  Serial.print("[WiFi] ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

// ---------- MQTT ----------
void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("[MQTT] Connecting...");
    if (mqtt.connect(GATEWAY_ID)) {
      Serial.println("OK");
    } else {
      Serial.println("FAILED, retrying...");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== ESP32 BLE → MQTT (NON-TLS) ===");

  connectWiFi();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  connectMQTT();

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(false);   // IMPORTANT
  pBLEScan->setInterval(200);
  pBLEScan->setWindow(50);

  beacon.valid = false;
}

void loop() {

  mqtt.loop();

  // ---- BLE SCAN ----
  if (millis() - lastScan > 3000) {
    beacon.valid = false;
    Serial.println("[BLE] Scanning...");
    pBLEScan->start(2, false);
    pBLEScan->clearResults();
    lastScan = millis();
  }

  // ---- MQTT PUBLISH ----
  if (beacon.valid && millis() - lastPublish > 2000) {

    StaticJsonDocument<128> doc;
    doc["gateway_id"] = GATEWAY_ID;
    doc["major"] = beacon.major;
    doc["minor"] = beacon.minor;
    doc["rssi"]  = beacon.rssi;
    doc["timestamp"]    = millis();

    char payload[128];
    serializeJson(doc, payload);

    mqtt.publish(MQTT_TOPIC, payload);
    Serial.println("[MQTT] Sent");

    beacon.valid = false;
    lastPublish = millis();
  }
}
