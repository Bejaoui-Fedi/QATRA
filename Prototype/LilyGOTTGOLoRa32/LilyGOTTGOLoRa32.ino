#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RadioLib.h>

// ---------- OLED ----------
#define OLED_SDA 21
#define OLED_SCL 22

// ---------- LoRa SX1276 (TTGO LoRa32 V1.6 / T3) ----------
#define LORA_SCK  5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_NSS  18
#define LORA_RST  23   // mettre 14 si votre carte est une V1 / V2.0
#define LORA_DIO0 26
#define LORA_DIO1 33

Adafruit_SSD1306 display(128, 64, &Wire, -1);
SX1276 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RST, LORA_DIO1);

// Affiche le titre en grand + une ligne d'état en bas
void afficherEcran(String statut) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 4);
  display.println("Sollitech");
  display.setCursor(0, 24);
  display.println("Winner");

  display.setTextSize(1);
  display.setCursor(0, 50);
  display.println(statut);
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erreur OLED");
    while (true) delay(1000);
  }

  afficherEcran("Demarrage...");

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);

  ConfigLoRa_t cfg;
  cfg.frequency = RADIOLIB_UNIT_MEGA(868.8);
  cfg.bandwidth = RADIOLIB_UNIT_KILO(125);   // ou 125000, selon ce qui a marche chez vous
  cfg.spreadingFactor = 7;
  cfg.codingRate = 5;
  cfg.syncWord = 0x12;
  cfg.preambleLength = 8;

  int state = radio.begin(cfg);
  if (state == RADIOLIB_ERR_NONE) {
    radio.setOutputPower(14);
    radio.setCRC(true);   // supprimez cette ligne si le compilateur la refuse
    afficherEcran("LoRa OK - ecoute");
    Serial.println("Sollitech Winner - LoRa pret sur 868.8 MHz");
  } else {
    afficherEcran("Erreur LoRa: " + String(state));
    Serial.println("Erreur LoRa: " + String(state));
    while (true) delay(1000);
  }
}

void loop() {
  String messageRecu;
  int state = radio.receive(messageRecu);

  if (state == RADIOLIB_ERR_NONE) {
    float rssi = radio.getRSSI();
    if (rssi > -120) {   // ignore le bruit
      Serial.println("Recu : " + messageRecu + " | RSSI: " + String(rssi) + " dBm");
      afficherEcran("Recu: " + messageRecu);
    }
  } else if (state != RADIOLIB_ERR_RX_TIMEOUT && state != RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("Erreur reception: " + String(state));
  }
}