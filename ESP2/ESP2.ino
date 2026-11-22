#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>

// ---------- WiFi / MQTT (a modifier) ----------
#define WIFI_SSID  "VOTRE_WIFI"           // Wokwi : "Wokwi-GUEST"
#define WIFI_PASS  "VOTRE_MOT_DE_PASSE"   // Wokwi : ""
#define MQTT_HOST  "broker.hivemq.com"
#define MQTT_PORT  1883
#define TOPIC_BASE "qatra/solitech42"     // meme valeur que sur ESP1
#define T_DATA     TOPIC_BASE "/capteurs"
#define T_STATUS   TOPIC_BASE "/esp1/statut"
#define T_STATE    TOPIC_BASE "/etat"
#define T_CMD      TOPIC_BASE "/cmd"

#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 4

#define SPLASH_NAME "QATRA"
#define SPLASH_TEAM "SOLITECH"

// ---------- Etiquette de la carte -> GPIO (A VERIFIER pour ton modele) ----------
#define P_D0 21
#define P_D1 22
#define P_D2 27
#define P_D3 23
#define P_D4 19
#define P_D5 18
#define P_D6 25
#define P_D7 26
#define P_D8 13
#define P_D9 14
#define P_A0 32
#define P_A1 33

// ---------- Broches utilisees (noms de la carte) ----------
#define PIN_SDA     P_D0   // LCD / PCF8574 SDA
#define PIN_SCL     P_D1   // LCD / PCF8574 SCL
#define SD_CS       P_D2   // module SD : CS
#define SD_MOSI     P_D3   // module SD : SI
#define SD_MISO     P_D4   // module SD : SO
#define SD_SCK      P_D5   // module SD : SCK
#define LED_TEMP    P_D6   // rouge
#define LED_SOIL    P_D7   // jaune
#define LED_WATER   P_D8   // bleue
#define LED_OK      P_D9   // verte
#define BUZZER_PIN  P_A0   // via Q1 + 1k
#define PUMP_PIN    P_A1   // via Q2 + 1k + relais
#define PUMP_ON_LEVEL HIGH

// ---------- Seuils d'alerte ----------
const float TEMP_MAX   = 30.0;
const float TEMP_CRIT  = 35.0;
const int   SOIL_CRIT  = 15;
const int   SOIL_MIN   = 30;
const int   SOIL_WETOK = 60;
const int   SOIL_SAT   = 90;
const int   WATER_CRIT = 10;
const int   WATER_MIN  = 40;
const float FLOW_ON    = 0.5;
const float FLOW_MAX   = 10.0;

// ---------- Pompe ----------
const int   SOIL_START       = 30;     // demarrage si sol < 30 %
const int   SOIL_STOP        = 45;     // arret a 45 % (hysteresis)
const int   PUMP_WATER_START = 25;     // demarrage interdit si reservoir < 25 %
const int   PUMP_WATER_STOP  = 10;     // arret si reservoir < 10 %
const float FLOW_MIN_RUN     = 0.3;    // debit minimal attendu pompe en marche
const unsigned long PUMP_MAX_RUN  = 60000UL;
const unsigned long PUMP_REST     = 30000UL;
const unsigned long NOFLOW_GRACE  = 6000UL;
const uint8_t       NOFLOW_FRAMES = 2;
const unsigned long FLOW_SETTLE   = 8000UL;
const unsigned long FAULT_LOCK_MS = 300000UL;   // 0 = jusqu'au redemarrage

// ---------- Liaison, affichage, SD ----------
const unsigned long LINK_TIMEOUT = 6000UL;
const unsigned long MSG_INTERVAL = 2000UL;
const unsigned long LOG_INTERVAL = 10000UL;
const unsigned long SD_RETRY     = 30000UL;
const unsigned long STATE_PUB    = 5000UL;
const char LOG_FILE[] = "/QATRA.CSV";

// ---------- Etats ----------
enum {
  ST_NONE, ST_PUMP, ST_LINK, ST_SENSOR, ST_LEAK, ST_PUMPFAULT, ST_TANKEMPTY,
  ST_TEMPCRIT, ST_STRESS, ST_SOILCRIT, ST_TIMEOUT, ST_TEMPHIGH, ST_SOILDRY,
  ST_TANKLOW, ST_WASTE, ST_SATURATED, ST_FLOWOFF
};

const char* stText(uint8_t s) {
  switch (s) {
    case ST_PUMP:      return "POMPE ACTIVE";
    case ST_LINK:      return "LIAISON PERDUE";
    case ST_SENSOR:    return "ERREUR CAPTEUR";
    case ST_LEAK:      return "FUITE DEBIT!";
    case ST_PUMPFAULT: return "POMPE SANS DEBIT";
    case ST_TANKEMPTY: return "RESERVOIR VIDE";
    case ST_TEMPCRIT:  return "TEMP CRITIQUE";
    case ST_STRESS:    return "STRESS HYDRIQUE";
    case ST_SOILCRIT:  return "SOL TRES SEC";
    case ST_TIMEOUT:   return "TIMEOUT POMPE";
    case ST_TEMPHIGH:  return "TEMP ELEVEE";
    case ST_SOILDRY:   return "SOL SEC";
    case ST_TANKLOW:   return "EAU BASSE";
    case ST_WASTE:     return "GASPILLAGE";
    case ST_SATURATED: return "SOL SATURE";
    case ST_FLOWOFF:   return "DEBIT HORS POMPE";
    default:           return "NORMAL";
  }
}

enum { PS_IDLE, PS_RUN, PS_REST, PS_FAULT };
#define LM_T  1
#define LM_S  2
#define LM_W  4
#define LM_OK 8

// ---------- Variables ----------
WiFiClient   wifi;
PubSubClient mqtt(wifi);
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
const uint8_t ROW_OFFSET[4] = {0x00, 0x40, 0x10, 0x50};   // LCD 16x4

float tempVal = 0, humVal = 0, flowVal = 0, volumeL = 0;
int   soilVal = 0, waterVal = 0;
bool  haveData = false, sensorFault = false, linkLost = false, esp1Offline = false;
bool  forceManual = false;
unsigned long lastFrame = 0;
uint16_t badFrames = 0;

uint8_t curStatus = ST_NONE, curSev = 0, ledMask = 0;

uint8_t pumpState = PS_IDLE;
unsigned long pumpStart = 0, pumpStop = 0;
bool    pumpTimedOut = false;
uint8_t noFlowCount = 0, flowOffCount = 0;
bool    flowWhileOff = false;

bool sdOk = false;
unsigned long lastLog = 0, lastSdTry = 0, lastMqttTry = 0, lastStatePub = 0;
uint8_t lastLoggedStatus = 255, lastPubStatus = 255;

bool msgPhase = false;
unsigned long lastPhase = 0, lastLcd = 0;
bool lcdDirty = true;

// ======================= LCD =======================
void lcdLine(uint8_t row, const char *s, bool center = false) {
  char b[LCD_COLS + 1];
  uint8_t n = strlen(s);
  if (n > LCD_COLS) n = LCD_COLS;
  memset(b, ' ', LCD_COLS);
  b[LCD_COLS] = 0;
  uint8_t off = center ? (LCD_COLS - n) / 2 : 0;
  memcpy(b + off, s, n);
  lcd.command(0x80 | ROW_OFFSET[row]);
  lcd.print(b);
}

void lcdRefresh() {
  char b[32], l1[LCD_COLS + 1];

  if (haveData) snprintf(b, sizeof(b), "T:%.1fC H:%.0f%%", tempVal, humVal);
  else          strcpy(b, "T:--.-C H:--%");
  memset(l1, ' ', LCD_COLS);
  l1[LCD_COLS] = 0;
  uint8_t n = strlen(b);
  if (n > LCD_COLS - 1) n = LCD_COLS - 1;
  memcpy(l1, b, n);
  l1[LCD_COLS - 1] = sdOk ? 'S' : '!';
  lcdLine(0, l1);

  if (!haveData) {
    lcdLine(1, "Sol: -- %");
    lcdLine(2, "Eau: -- %");
  } else {
    const char *ss = (soilVal < 30) ? "SEC" : (soilVal <= 60) ? "HUMIDE" : "MOUILLE";
    snprintf(b, sizeof(b), "Sol: %s %d%%", ss, soilVal);
    if (strlen(b) > LCD_COLS) snprintf(b, sizeof(b), "Sol:%s %d%%", ss, soilVal);
    lcdLine(1, b);

    const char *ws = (waterVal < 10) ? "VIDE" : (waterVal < 40) ? "BAS" : (waterVal <= 70) ? "MOYEN" : "HAUT";
    snprintf(b, sizeof(b), "Eau: %s %d%%", ws, waterVal);
    lcdLine(2, b);
  }

  if (linkLost)         lcdLine(3, "LIAISON PERDUE", true);
  else if (sensorFault) lcdLine(3, "ERREUR CAPTEUR", true);
  else if (!haveData)   lcdLine(3, mqtt.connected() ? "Deb: -- L/min" : "MQTT HORS LIGNE", !mqtt.connected());
  else if (!msgPhase) {
    snprintf(b, sizeof(b), "Deb: %.1f L/min", flowVal);
    lcdLine(3, b);
  } else if (curStatus != ST_NONE) {
    lcdLine(3, stText(curStatus), true);
  } else {
    snprintf(b, sizeof(b), "Vol: %.2f L", volumeL);
    lcdLine(3, b);
  }
}

// ======================= EVALUATION =======================
void setStatus(uint8_t st, uint8_t sev) {
  if (curStatus == ST_NONE || sev > curSev) { curStatus = st; curSev = sev; }
}

void evaluate() {
  curStatus = ST_NONE;
  curSev = 0;
  ledMask = 0;

  if (linkLost)    { setStatus(ST_LINK, 2);   ledMask = LM_T | LM_S | LM_W | LM_OK; return; }
  if (sensorFault) { setStatus(ST_SENSOR, 2); ledMask = LM_T | LM_S; return; }
  if (!haveData)   { return; }

  bool pumping = (pumpState == PS_RUN);

  // Critiques (par ordre d'importance)
  if (flowVal > FLOW_MAX)      { setStatus(ST_LEAK, 2);      ledMask |= LM_T | LM_W; }
  if (pumpState == PS_FAULT)   { setStatus(ST_PUMPFAULT, 2); ledMask |= LM_S | LM_W; }
  if (waterVal < WATER_CRIT)   { setStatus(ST_TANKEMPTY, 2); ledMask |= LM_W; }
  if (tempVal > TEMP_CRIT)     { setStatus(ST_TEMPCRIT, 2);  ledMask |= LM_T; }
  if (tempVal > TEMP_MAX && soilVal < SOIL_MIN && !pumping) { setStatus(ST_STRESS, 2); ledMask |= LM_T | LM_S; }
  if (soilVal < SOIL_CRIT && !pumping) { setStatus(ST_SOILCRIT, 2); ledMask |= LM_S; }

  // Avertissements
  if (pumpTimedOut)            { setStatus(ST_TIMEOUT, 1);   ledMask |= LM_S; }
  if (tempVal > TEMP_MAX)      { setStatus(ST_TEMPHIGH, 1);  ledMask |= LM_T; }
  if (soilVal < SOIL_MIN && !pumping) { setStatus(ST_SOILDRY, 1); ledMask |= LM_S; }
  if (waterVal < WATER_MIN)    { setStatus(ST_TANKLOW, 1);   ledMask |= LM_W; }
  if (flowVal >= FLOW_ON && soilVal > SOIL_WETOK) { setStatus(ST_WASTE, 1); ledMask |= LM_S; }
  if (soilVal > SOIL_SAT && flowVal < FLOW_ON)    { setStatus(ST_SATURATED, 1); ledMask |= LM_S; }
  if (flowWhileOff)            { setStatus(ST_FLOWOFF, 1);   ledMask |= LM_W; }

  bool alert = (ledMask != 0);
  if (pumping) setStatus(ST_PUMP, 0);
  if (!alert) ledMask = LM_OK | (pumping ? LM_S : 0);
}

void ledApply() {
  digitalWrite(LED_TEMP,  (ledMask & LM_T)  ? HIGH : LOW);
  digitalWrite(LED_SOIL,  (ledMask & LM_S)  ? HIGH : LOW);
  digitalWrite(LED_WATER, (ledMask & LM_W)  ? HIGH : LOW);
  digitalWrite(LED_OK,    (ledMask & LM_OK) ? HIGH : LOW);
}

void buzzerTick() {
  unsigned long t = millis();
  bool on = false;
  if (curSev == 2) {                         // critique : 3 bips rapides / 2 s
    unsigned long p = t % 2000;
    on = (p < 150) || (p >= 300 && p < 450) || (p >= 600 && p < 750);
  } else if (curSev == 1) {                  // avertissement : 1 bip / 10 s
    on = (t % 10000) < 100;
  }
  digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
}

// ======================= POMPE =======================
void pumpDrive(bool on) {
  digitalWrite(PUMP_PIN, on ? PUMP_ON_LEVEL : !PUMP_ON_LEVEL);
}

void pumpStopNow(uint8_t newState) {
  pumpDrive(false);
  pumpState = newState;
  pumpStop = millis();
  flowOffCount = 0;
  flowWhileOff = false;
}

void pumpUpdate() {
  unsigned long t = millis();
  bool dataOk = haveData && !linkLost && !sensorFault;
  bool leak = dataOk && (flowVal > FLOW_MAX);

  switch (pumpState) {
    case PS_IDLE:
      if (dataOk && !leak && waterVal >= PUMP_WATER_START &&
          (soilVal < SOIL_START || forceManual)) {
        pumpState = PS_RUN;
        pumpStart = t;
        pumpTimedOut = false;
        noFlowCount = 0;
        pumpDrive(true);
      }
      break;

    case PS_RUN:
      if (!dataOk || leak || waterVal < PUMP_WATER_STOP ||
          (!forceManual && soilVal >= SOIL_STOP)) {
        pumpStopNow(PS_REST);
      } else if (t - pumpStart >= PUMP_MAX_RUN) {
        pumpTimedOut = true;
        pumpStopNow(PS_REST);
      }
      break;

    case PS_REST:
      if (soilVal >= SOIL_STOP) pumpTimedOut = false;
      if (t - pumpStop >= PUMP_REST) pumpState = PS_IDLE;
      break;

    case PS_FAULT:
      if (FAULT_LOCK_MS > 0 && t - pumpStop >= FAULT_LOCK_MS) {
        pumpState = PS_IDLE;
        noFlowCount = 0;
      }
      break;
  }
}

void flowChecks() {
  unsigned long t = millis();

  if (pumpState == PS_RUN && (t - pumpStart) > NOFLOW_GRACE && flowVal < FLOW_MIN_RUN) {
    if (++noFlowCount >= NOFLOW_FRAMES) pumpStopNow(PS_FAULT);
  } else {
    noFlowCount = 0;
  }

  if (pumpState != PS_RUN && (t - pumpStop) > FLOW_SETTLE && flowVal >= FLOW_ON) {
    if (flowOffCount < 255) flowOffCount++;
  } else {
    flowOffCount = 0;
  }
  flowWhileOff = (flowOffCount >= 2);
}

// ======================= SD =======================
bool openSd() {
  if (!SD.begin(SD_CS, SPI)) return false;
  if (!SD.exists(LOG_FILE)) {
    File f = SD.open(LOG_FILE, FILE_WRITE);
    if (!f) return false;
    f.println("temps_s,temp_C,hum_air_pct,sol_pct,eau_pct,debit_Lmin,volume_L,pompe,etat");
    f.close();
  }
  return true;
}

void logData() {
  File f = SD.open(LOG_FILE, FILE_APPEND);
  if (!f) { sdOk = false; lcdDirty = true; return; }
  f.printf("%lu,%.1f,%.1f,%d,%d,%.1f,%.2f,%d,%s\n",
           millis() / 1000UL, tempVal, humVal, soilVal, waterVal, flowVal, volumeL,
           pumpState == PS_RUN ? 1 : 0, stText(curStatus));
  f.close();
}

void logTick() {
  unsigned long t = millis();
  if (!sdOk && t - lastSdTry >= SD_RETRY) {
    lastSdTry = t;
    sdOk = openSd();
    lcdDirty = true;
  }
  if (!sdOk || !haveData) return;

  if (curStatus != lastLoggedStatus || t - lastLog >= LOG_INTERVAL) {
    lastLog = t;
    lastLoggedStatus = curStatus;
    logData();
  }
}

// ======================= MQTT =======================
bool field(const char *s, char key, float &out) {
  for (const char *p = s; *p; p++) {
    if ((p == s || *(p - 1) == ',') && *p == key && *(p + 1) == ':') {
      char *end;
      out = strtod(p + 2, &end);
      return end != p + 2;
    }
  }
  return false;
}

void handleFrame(const char *line) {
  unsigned long now = millis();

  if (line[0] == 'E' && line[1] == ':') {       // erreur capteur cote ESP1
    lastFrame = now;
    linkLost = false;
    sensorFault = true;
    lcdDirty = true;
    return;
  }

  float t, h, s, w, f;
  if (!field(line, 'T', t) || !field(line, 'H', h) || !field(line, 'S', s) ||
      !field(line, 'W', w) || !field(line, 'F', f)) { badFrames++; return; }

  if (t < -40 || t > 85 || h < 0 || h > 100 || s < 0 || s > 100 ||
      w < 0 || w > 100 || f < 0 || f > 30) { badFrames++; return; }

  // Volume : le debit recu est la moyenne sur l'intervalle ecoule
  if (haveData && !linkLost) {
    float dt = (now - lastFrame) / 1000.0;
    if (dt > 5.0) dt = 5.0;
    volumeL += f / 60.0 * dt;
  }

  tempVal = t;  humVal = h;
  soilVal = (int)(s + 0.5);
  waterVal = (int)(w + 0.5);
  flowVal = f;

  haveData = true;
  sensorFault = false;
  linkLost = false;
  esp1Offline = false;
  lastFrame = now;

  flowChecks();
  lcdDirty = true;
}

void onMessage(char *topic, byte *payload, unsigned int len) {
  char buf[96];
  if (len >= sizeof(buf)) return;
  memcpy(buf, payload, len);
  buf[len] = 0;

  if (strcmp(topic, T_DATA) == 0) {
    handleFrame(buf);
  } else if (strcmp(topic, T_STATUS) == 0) {
    esp1Offline = (strcmp(buf, "offline") == 0);
    if (esp1Offline) { linkLost = true; lcdDirty = true; }
    else lastFrame = millis();
  } else if (strcmp(topic, T_CMD) == 0) {
    // Test de la pompe : les protections restent actives
    if (strcmp(buf, "FORCE:1") == 0) forceManual = true;
    else if (strcmp(buf, "FORCE:0") == 0) forceManual = false;
  }
}

void mqttEnsure() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  if (millis() - lastMqttTry < 5000) return;
  lastMqttTry = millis();

  char id[32];
  snprintf(id, sizeof(id), "qatra-esp2-%04X", (uint16_t)(ESP.getEfuseMac() >> 32));
  if (mqtt.connect(id)) {
    mqtt.subscribe(T_DATA);
    mqtt.subscribe(T_STATUS);
    mqtt.subscribe(T_CMD);
    lcdDirty = true;
  }
}

void publishState() {
  if (!mqtt.connected()) return;
  unsigned long t = millis();
  if (curStatus == lastPubStatus && t - lastStatePub < STATE_PUB) return;
  lastStatePub = t;
  lastPubStatus = curStatus;

  char buf[200];
  snprintf(buf, sizeof(buf),
           "{\"statut\":\"%s\",\"pompe\":%d,\"temp\":%.1f,\"hum\":%.1f,\"sol\":%d,\"eau\":%d,\"debit\":%.1f,\"volume\":%.2f}",
           stText(curStatus), pumpState == PS_RUN ? 1 : 0, tempVal, humVal,
           soilVal, waterVal, flowVal, volumeL);
  mqtt.publish(T_STATE, buf);
}

// ======================= DEMARRAGE =======================
void showSplash() {
  sdOk = openSd();
  lastSdTry = millis();

  lcd.clear();
  lcdLine(0, SPLASH_NAME, true);
  lcdLine(1, sdOk ? "SD: OK" : "SD: ERREUR", true);
  lcdLine(2, "Realised by", true);
  lcdLine(3, SPLASH_TEAM, true);

  digitalWrite(BUZZER_PIN, HIGH);   // bip de demarrage
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  delay(2800);
  lcd.clear();
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_TEMP,   OUTPUT);
  pinMode(LED_SOIL,   OUTPUT);
  pinMode(LED_WATER,  OUTPUT);
  pinMode(LED_OK,     OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PUMP_PIN,   OUTPUT);
  pumpDrive(false);                 // pompe arretee au demarrage
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(PIN_SDA, PIN_SCL);
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  lcd.init();
  lcd.backlight();
  showSplash();

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(256);

  lastFrame = millis();
  pumpStop = millis() - PUMP_REST;
  lastLog = millis() - LOG_INTERVAL;
  lastMqttTry = millis() - 5000;
  evaluate();
  ledApply();
  lcdRefresh();
}

void loop() {
  unsigned long t = millis();

  mqttEnsure();
  mqtt.loop();

  // Chien de garde : donnees d'ESP1 (timeout ou testament MQTT)
  bool lost = esp1Offline || (t - lastFrame > LINK_TIMEOUT);
  if (lost != linkLost) { linkLost = lost; lcdDirty = true; }

  pumpUpdate();

  uint8_t prevStatus = curStatus;
  evaluate();
  if (curStatus != prevStatus) lcdDirty = true;
  ledApply();

  if (t - lastPhase >= MSG_INTERVAL) {
    lastPhase = t;
    msgPhase = !msgPhase;
    lcdDirty = true;
  }

  if (lcdDirty && t - lastLcd >= 200UL) {
    lastLcd = t;
    lcdDirty = false;
    lcdRefresh();
  }

  logTick();
  publishState();
  buzzerTick();
}