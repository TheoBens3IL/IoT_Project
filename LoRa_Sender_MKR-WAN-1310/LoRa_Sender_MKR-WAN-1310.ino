// Type de carte : Arduino MKR WAN 1310
// Librairie fonctionnelle pour le MKR IoT CARRIER REV2 : Adafruit_BME680
// Librairies non fonctionnelles                        : BME280, BMP3XX, HTS221, Arduino_MKRIoTCarrier

#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_BME680.h> // Adafruit BME680 Library v2.0.5

// ====================
// LoRa Parameters
// ====================
#define LORA_FREQ     868E6
#define LORA_SF       12
#define LORA_BW       125E3
#define LORA_TX_POWER 17

// ====================
// Capteur BME680 (I2C)
// ===================
Adafruit_BME680 bme; // I2C - MKR IoT CARRIER REV2

// ====================
// Setup
// ====================
void setup() {
  Serial.begin(9600);
  while (!Serial);

  // ===== Init Carrier BME680 ===== //
  if (!bme.begin(0x76)) {
    Serial.println("Erreur BME680 !");
    while (1);
  }

  // Paramétrage des mesures
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);

  Serial.println("BME680 OK");

  // ==== Init Lora ===== //
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setTxPower(LORA_TX_POWER);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.enableCrc();

  Serial.println("LoRa Setup OK ");
}

// ====================
// Loop
// ====================
void loop() {
  // Lancement d'une mesure complète
  if (!bme.performReading()) {
    Serial.println("Erreur lecture BME680");
    return;
  }

  // Récupération des mesures
  float temperature = bme.temperature;       // °C
  float humidity    = bme.humidity;          // %
  float pressure    = bme.pressure / 100.0;  // hPa (Pa → hPa)

  // Affichage dans le Serial Monitor
  Serial.println("-------------------------------------");
  Serial.print("Température : ");
  Serial.print(temperature, 1);
  Serial.println(" °C");

  Serial.print("Humidité    : ");
  Serial.print(humidity, 1);
  Serial.println(" %");

  Serial.print("Pression    : ");
  Serial.print(pressure, 1);
  Serial.println(" hPa");

  // Préparation du payload LoRa
  char payload[64];
  snprintf(payload, sizeof(payload),
           "Temp=%.1f,Hum=%.1f,Press=%.1f",
           temperature, humidity, pressure);

  Serial.print("Sending: ");
  Serial.println(payload);

  // Envoi LoRa
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();

  delay(5000);
}