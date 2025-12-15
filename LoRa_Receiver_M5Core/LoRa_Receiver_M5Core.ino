// Ajouter au gestionnaire des cartes : https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
// Type de carte : M5Core

#include <SPI.h>
#include "M5_SX127X.h"        // M5_SX127x v1.0.0
#include <WiFi.h>
#include <MQTTPubSubClient.h> // ArduinoMqttClient v0.1.8

// -----------------------
// LoRa
// -----------------------
// Module Connect Pins Config
#define CS_PIN    5  // 5 12 15 0
#define RST_PIN   13 // 13 25
#define IRQ_PIN   34 // 34 35
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SCLK 18

// LoRa Parameters Config
#define LORA_FREQ     868E6
#define LORA_SF       12
#define LORA_BW       125E3
#define LORA_TX_POWER 17

// -----------------------
// WIFI
// -----------------------
const char* ssid = "wifi_name";
const char* password = "wifi_password";

// -----------------------
// ADAFRUIT IO
// -----------------------
#define IO_USERNAME "adafruit_username"
#define IO_KEY      "adafruit_password"

#define FEED_LORA_TEMP  "adafruit_username/feeds/LoRa_Temperature"
#define FEED_LORA_HUM   "adafruit_username/feeds/LoRa_Humidite"
#define FEED_LORA_PRESS "adafruit_username/feeds/LoRa_Pression"

WiFiClient wifiClient;
MQTTPubSubClient mqttClient;

// -----------------------
// Setup
// -----------------------
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("LoRa Receiver Starting...");

    //M5.begin(); <-- à ne pas faire

    // Init SPI + LoRa
    Serial.println("LoRa Receiver ");
    SPI.begin(LORA_SCLK, LORA_MISO, LORA_MOSI, -1);
    LoRa.setSPI(&SPI);
    LoRa.setPins(CS_PIN, RST_PIN, IRQ_PIN);

    while (!LoRa.begin(LORA_FREQ)) {
        Serial.println("LoRa init fail.");
        delay(3000);
    }

    LoRa.setTxPower(LORA_TX_POWER);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setSpreadingFactor(LORA_SF);

    Serial.println("LoRa Setup OK ");

    // Connexion WiFi
    Serial.println("Connexion WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    }
    Serial.println("\nWiFi OK.");

    // Connexion MQTT
    Serial.println("Connexion MQTT...");
    if (!wifiClient.connect("io.adafruit.com", 1883)) {
    Serial.println("TCP fail");
    while (1);
    }
    mqttClient.begin(wifiClient);
    while (!mqttClient.connect("Core2Device", IO_USERNAME, IO_KEY)) {
    Serial.println("Retry MQTT...");
    delay(1500);
    }
    Serial.println("MQTT OK.");
}

// -----------------------
// Loop
// -----------------------
void loop() {
  mqttClient.update();

  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  // Lecture du payload LoRa
  String payload;
  while (LoRa.available()) {
    payload += (char)LoRa.read();
  }

  Serial.print("Received: ");
  Serial.println(payload);

  // Variables extraites
  float temperature = NAN;
  float humidity    = NAN;
  float pressure    = NAN;

  // Parsing du payload : Temp=xx,Hum=yy,Press=zz
  int tIndex = payload.indexOf("Temp=");
  int hIndex = payload.indexOf("Hum=");
  int pIndex = payload.indexOf("Press=");

  if (tIndex >= 0) {
    temperature = payload.substring(
      tIndex + 5,
      payload.indexOf(',', tIndex)
    ).toFloat();
  }

  if (hIndex >= 0) {
    humidity = payload.substring(
      hIndex + 4,
      payload.indexOf(',', hIndex)
    ).toFloat();
  }

  if (pIndex >= 0) {
    pressure = payload.substring(
      pIndex + 6
    ).toFloat();
  }

  // Publication MQTT séparée
  if (!isnan(temperature)) {
    mqttClient.publish(FEED_LORA_TEMP, String(temperature, 1).c_str());
    Serial.print("→ Temp published: ");
    Serial.println(temperature);
  }

  if (!isnan(humidity)) {
    mqttClient.publish(FEED_LORA_HUM, String(humidity, 1).c_str());
    Serial.print("→ Hum published: ");
    Serial.println(humidity);
  }

  if (!isnan(pressure)) {
    mqttClient.publish(FEED_LORA_PRESS, String(pressure, 1).c_str());
    Serial.print("→ Press published: ");
    Serial.println(pressure);
  }

  Serial.println("-----------------------------------");
}