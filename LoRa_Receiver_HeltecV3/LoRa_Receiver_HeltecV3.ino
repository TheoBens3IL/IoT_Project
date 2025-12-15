#include <SPI.h>
#include <RadioLib.h>
#include <WiFi.h>
#include <MQTTPubSubClient.h>

// ======================
// LoRa Pins – Heltec V3
// ======================
#define LORA_SCK   9
#define LORA_MISO 11
#define LORA_MOSI 10
#define LORA_CS    8
#define LORA_RST  12
#define LORA_DIO1 14

#define LORA_FREQ 868.0  // MHz
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST);

// ======================
// WiFi / Adafruit IO
// ======================
const char* ssid = "wifi_name";
const char* password = "wifi_password";

#define IO_USERNAME "adafruit_username"
#define IO_KEY      "adafruit_password"

#define FEED_LORA_TEMP  "adafruit_username/feeds/LoRa_Temperature"
#define FEED_LORA_HUM   "adafruit_username/feeds/LoRa_Humidite"
#define FEED_LORA_PRESS "adafruit_username/feeds/LoRa_Pression"

WiFiClient wifiClient;
MQTTPubSubClient mqttClient;

// ======================
// Utilitaires parsing
// ======================
bool extractValue(const String& payload, const String& key, float& value) {
  int start = payload.indexOf(key);
  if (start == -1) return false;

  start += key.length();
  int end = payload.indexOf(',', start);
  if (end == -1) end = payload.length();

  value = payload.substring(start, end).toFloat();
  return true;
}

// ======================
// Setup
// ======================
void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("LoRa Receiver Heltec V3 + Adafruit IO");

  // LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  int state = radio.begin(LORA_FREQ, 125.0, 12, 5, 0x12, 17, 8);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("LoRa init failed: ");
    Serial.println(state);
    while (1);
  }
  Serial.println("LoRa OK");

  // WiFi
  Serial.print("Connexion WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK");

  // MQTT
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

// ======================
// Loop
// ======================
void loop() {
  mqttClient.update();

  String payload;
  int state = radio.receive(payload);
  if (state == RADIOLIB_ERR_NONE) {
    int rssi = radio.getRSSI();  // puissance du signal reçu (en dBm) : plus il est proche de 0, plus le signal est fort

    float temperature = NAN;
    float humidity    = NAN;
    float pressure    = NAN;

    extractValue(payload, "Temp=", temperature);
    extractValue(payload, "Hum=",  humidity);
    extractValue(payload, "Press=", pressure);

    Serial.println("-----------------------------------");
    Serial.print("Payload brut : ");
    Serial.println(payload);

    if (!isnan(temperature)) {
      Serial.print("Température : "); Serial.println(temperature);
      mqttClient.publish(FEED_LORA_TEMP, String(temperature, 1).c_str());
    }

    if (!isnan(humidity)) {
      Serial.print("Humidité    : "); Serial.println(humidity);
      mqttClient.publish(FEED_LORA_HUM, String(humidity, 1).c_str());
    }

    if (!isnan(pressure)) {
      Serial.print("Pression    : "); Serial.println(pressure);
      mqttClient.publish(FEED_LORA_PRESS, String(pressure, 1).c_str());
    }

    Serial.print("RSSI        : "); Serial.println(rssi);
    Serial.println("-----------------------------------\n");
  }
}

