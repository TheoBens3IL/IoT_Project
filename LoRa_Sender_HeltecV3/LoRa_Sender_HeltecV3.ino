// Pour détecter la carte Heltec en port USB, installer le driver CP210x Windows Drivers : https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads
// Ajouter au gestionnaire des cartes : https://resource.heltec.cn/download/package_heltec_esp32_index.json
// Type de carte : Heltec WiFi LoRa 32(V3)

// Librairies pour la tranmission LoRa
#include <SPI.h>
#include <RadioLib.h>        // RadioLib v7.4.0 (Librairie LoRa SX1262 pour la communication radio)
// Librairies pour le capteur SEN-KKY015TF DHT11
#include <Adafruit_Sensor.h> // (Librairie de base pour capteurs Adafruit)
#include <DHT.h>             // DHT sensor library v1.4.6 (Librairie DHT pour capteurs DHT11)

// ====================
// Configuration DHT11
// ====================
#define DHTPIN 2             // Broche de données du DHT11 connectée à GPIO2
#define DHTTYPE DHT11        // Type de capteur DHT
DHT dht(DHTPIN, DHTTYPE);    // Initialisation de l’objet DHT

// ====================
// Configuration LoRa SX1262
// ====================
#define LORA_SCK   9         // SPI Clock
#define LORA_MISO 11         // SPI MISO
#define LORA_MOSI 10         // SPI MOSI
#define LORA_CS    8         // Chip Select
#define LORA_RST  12         // Reset du module
#define LORA_BUSY 13         // Pin BUSY pour SX1262
#define LORA_DIO1 14         // Pin DIO1 utilisée pour les interruptions

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY); // Objet RadioLib

// Paramètres LoRa
#define LORA_FREQ     868.0  // Fréquence LoRa (MHz)
#define LORA_BW       125.0  // Largeur de bande LoRa (kHz)
#define LORA_SF       12     // Facteur d’étalement
#define LORA_CR       5      // Codage rate
#define LORA_TX_POWER 17     // Puissance d’émission (dBm)

// ====================
// Setup
// ====================
void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("Heltec V3.2 DHT11 + LoRa SX1262 Starting...");

  // Initialisation DHT11
  dht.begin();
  Serial.println("DHT11 OK");

  // Initialisation SPI pour LoRa SX1262
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  
  // Initialisation LoRa
  Serial.println("Init SX1262...");
  int state = radio.begin(
    LORA_FREQ,
    LORA_BW,
    LORA_SF,
    LORA_CR,
    0x12,        // sync word LoRa privé
    LORA_TX_POWER,
    8            // preamble length
  );

  // Vérification si l’init LoRa a réussi
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("LoRa init failed: ");
    Serial.println(state);
    while (1);   // Boucle infinie si échec
  }
  Serial.println("LoRa OK");
}

// ====================
// Loop
// ====================
void loop() {
  // Lecture DHT11 de la température et de l’humidité
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Vérification des erreurs de lecture
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Erreur lecture DHT11 !");
    delay(2000);
    return;
  }

  // Préparation du message à envoyer
  char payload[32];
  snprintf(payload, sizeof(payload), "Temp=%.1f,Hum=%.1f", temperature, humidity);

  // Affichage du message dans la console
  Serial.print("Sending: ");
  Serial.println(payload);

  // Transmission via LoRa
  int state = radio.transmit(payload);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Send OK");
  } else {
    Serial.print("Send failed: ");
    Serial.println(state);
  }

  // Attente 5 secondes avant prochaine mesure
  delay(5000);
}

