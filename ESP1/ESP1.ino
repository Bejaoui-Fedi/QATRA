#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

// ---------- WiFi / MQTT (a modifier) ----------
#define WIFI_SSID  "VOTRE_WIFI"           // Wokwi : "Wokwi-GUEST"
#define WIFI_PASS  "VOTRE_MOT_DE_PASSE"   // Wokwi : ""
#define MQTT_HOST  "broker.hivemq.com"
#define MQTT_PORT  1883
#define TOPIC_BASE "qatra/solitech42"     // a rendre unique
#define T_DATA     TOPIC_BASE "/capteurs"
#define T_STATUS   TOPIC_BASE "/esp1/statut"

// ---------- Etiquette de la carte -> GPIO (A VERIFIER pour ton modele) ----------
#define P_D0 21
#define P_D1 22
#define P_D2 27
#define P_A0 32
#define P_A1 33

// ---------- Broches utilisees (noms de la carte) ----------
#define PIN_SDA   P_D0    // SHT21 SDA
#define PIN_SCL   P_D1    // SHT21 SCL
#define FLOW_PIN  P_D2    // YF-S201 (jaune, via diviseur 10k/20k)
#define SOIL_PIN  P_A0    // capteur de sol
#define WATER_PIN P_A1    // capteur de niveau d'eau

#define SHT21_ADDR 0x40
#define CMD_TEMP   0xE3
#define CMD_HUM    0xE5

// ---------- Calibration (valeurs brutes 0-4095) ----------
// Proteus/potentiometre : tension haute = humide -> DRY = 0, WET = 4095
// Vrai capteur capacitif : tension haute = sec   -> DRY ~ 2400, WET ~ 1200 (a mesurer)
const int SOIL_RAW_DRY    = 0;
const int SOIL_RAW_WET    = 4095;
const int WATER_RAW_EMPTY = 0;
const int WATER_RAW_FULL  = 4095;

const float FLOW_K = 7.5;                       // F(Hz) = 7.5 x debit (L/min)
const unsigned long SEND_INTERVAL = 2000;

WiFiClient   wifi;
PubSubClient mqtt(wifi);

volatile uint32_t pulseCount = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
unsigned long lastSend = 0, lastFlowTime = 0, lastMqttTry = 0;

void IRAM_ATTR pulseISR() {
  portENTER_CRITICAL_ISR(&mux);
  pulseCount++;
  portEXIT_CRITICAL_ISR(&mux);
}

bool sht21ReadRaw(uint8_t cmd, uint16_t &raw) {
  Wire.beginTransmission(SHT21_ADDR);
  Wire.write(cmd);
  if (Wire.endTransmission() != 0) return false;
  delay(100);
  if (Wire.requestFrom((uint8_t)SHT21_ADDR, (uint8_t)3) != 3) return false;
  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  Wire.read();                                  // CRC ignore
  raw = (((uint16_t)msb << 8) | lsb) & 0xFFFC;
  return true;
}

int readAnalogAvg(uint8_t pin) {
  long sum = 0;
  for (int i = 0; i < 16; i++) { sum += analogRead(pin); delay(2); }
  return (int)(sum / 16);
}

int toPercent(int raw, int lo, int hi) {
  return constrain(map(raw, lo, hi, 0, 100), 0, 100);
}

float readFlow() {
  portENTER_CRITICAL(&mux);
  uint32_t p = pulseCount;
  pulseCount = 0;
  portEXIT_CRITICAL(&mux);

  unsigned long now = millis();
  float dt = (now - lastFlowTime) / 1000.0;
  lastFlowTime = now;
  if (dt < 0.2) return 0.0;
  return ((float)p / dt) / FLOW_K;
}

void mqttEnsure() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  if (millis() - lastMqttTry < 5000) return;
  lastMqttTry = millis();

  char id[32];
  snprintf(id, sizeof(id), "qatra-esp1-%04X", (uint16_t)(ESP.getEfuseMac() >> 32));
  // Testament : le broker publie "offline" si ESP1 disparait
  if (mqtt.connect(id, NULL, NULL, T_STATUS, 1, true, "offline")) {
    mqtt.publish(T_STATUS, "online", true);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setTimeOut(200);

  analogSetPinAttenuation(SOIL_PIN,  ADC_11db);
  analogSetPinAttenuation(WATER_PIN, ADC_11db);
  pinMode(FLOW_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseISR, RISING);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);

  lastFlowTime = millis();
  lastMqttTry  = millis() - 5000;
  lastSend     = millis() - SEND_INTERVAL;
}

void loop() {
  mqttEnsure();
  mqtt.loop();

  if (millis() - lastSend < SEND_INTERVAL) return;
  lastSend = millis();

  float flow = readFlow();

  char buf[80];
  uint16_t rawT, rawH;
  if (sht21ReadRaw(CMD_TEMP, rawT) && sht21ReadRaw(CMD_HUM, rawH)) {
    float temp = -46.85 + 175.72 * (float)rawT / 65536.0;
    float hum  = constrain(-6.0 + 125.0 * (float)rawH / 65536.0, 0.0, 100.0);
    int soil  = toPercent(readAnalogAvg(SOIL_PIN),  SOIL_RAW_DRY,    SOIL_RAW_WET);
    int water = toPercent(readAnalogAvg(WATER_PIN), WATER_RAW_EMPTY, WATER_RAW_FULL);
    snprintf(buf, sizeof(buf), "T:%.1f,H:%.1f,S:%d,W:%d,F:%.1f", temp, hum, soil, water, flow);
  } else {
    strcpy(buf, "E:SHT21");
  }

  Serial.println(buf);                          // debogage USB
  if (mqtt.connected()) mqtt.publish(T_DATA, buf);
}