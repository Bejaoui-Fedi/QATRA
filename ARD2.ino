#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ADDR 0x27
#define LCD_COLS 16

// ---------- Broches ----------
#define LED_TEMP   2   // rouge
#define LED_SOIL   3   // jaune
#define LED_WATER  4   // bleue
#define LED_OK     5   // verte
#define BUZZER_PIN 6   // buzzer
#define PUMP_PIN   7   // pompe (resistance 1k -> base de Q2 -> relais)
#define SD_CS      10  // carte SD (SCK=13, MISO=12, MOSI=11)

// ---------- Seuils (à ajuster) ----------
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
const unsigned long LINK_TIMEOUT = 6000;    // perte de liaison (ms)
const unsigned long MSG_INTERVAL = 2000;    // alternance débit / message (ms)
const unsigned long LOG_INTERVAL = 10000;   // enregistrement SD (ms)

// ---------- Pompe ----------
const int   SOIL_PUMP_OFF = 45;             // arrêt de la pompe quand le sol atteint 45 %
const unsigned long PUMP_MAX_RUN = 60000;   // durée maximale de marche (ms)
const unsigned long PUMP_REST    = 30000;   // repos minimal entre deux marches (ms)

const char LOG_FILE[] = "QATRA.CSV";

// ---------- Modes LED et motifs ----------
enum { M_OFF, M_ON, M_SLOW, M_FAST };
enum { P_NONE, P_LINK, P_ERR, P_LEAK };

uint8_t modeTemp = M_OFF, modeSoil = M_OFF, modeWater = M_OFF, modeOk = M_OFF;
uint8_t pattern  = P_NONE;
uint8_t severity = 0;          // 0 = normal, 1 = avertissement, 2 = critique

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 4);
const uint8_t ROW_OFFSET[4] = {0x00, 0x40, 0x10, 0x50};

// ---------- Données ----------
String lastTemp = "--.-";
String lastHum  = "--.-";
String flowStr  = "--";
float  tempVal  = 0, flowVal = 0;
int    soilVal  = 0, waterVal = 0;
bool   haveData = false;

String rxBuffer  = "";
String errorMsg  = "";
String statusMsg = "";
String lastLoggedMsg = "";

bool linkLost = false;
unsigned long lastFrame = 0;
unsigned long lastMsgSwitch = 0;
bool showMsgPhase = false;

// ---------- SD ----------
bool sdOk = false;
unsigned long lastLog = 0;

// ---------- Pompe ----------
bool pumpOn = false;
unsigned long pumpStart = 0;
unsigned long pumpStop  = 0;

// ---------- Affichage ----------
void setLine(uint8_t row, String s, bool center = false) {
  if (center) {
    int pad = (LCD_COLS - (int)s.length()) / 2;
    if (pad < 0) pad = 0;
    String sp = "";
    for (int i = 0; i < pad; i++) sp += ' ';
    s = sp + s;
  }
  while ((int)s.length() < LCD_COLS) s += ' ';
  s = s.substring(0, LCD_COLS);

  lcd.command(0x80 | ROW_OFFSET[row]);
  lcd.print(s);
}

String soilToState(int pct) {
  if (pct < 30)  return "SEC";
  if (pct <= 60) return "HUMIDE";
  return "MOUILLE";
}

String waterToState(int pct) {
  if (pct < 10)  return "VIDE";
  if (pct < 40)  return "BAS";
  if (pct <= 70) return "MOYEN";
  return "HAUT";
}

void refreshLine4() {
  if (linkLost) { setLine(3, "LIAISON PERDUE", true); return; }
  if (errorMsg != "") { setLine(3, errorMsg, true); return; }

  if (statusMsg != "" && showMsgPhase) {
    setLine(3, statusMsg, true);
  } else {
    if (flowStr == "--") setLine(3, "Deb: -- L/min");
    else setLine(3, "Deb: " + flowStr + " L/min");
  }
}

void showValues() {
  // Ligne 1 : mesures + indicateur SD en dernière colonne (S = OK, ! = problème)
  String l1 = "T:" + lastTemp + "C H:" + lastHum + "%";
  while (l1.length() < 15) l1 += ' ';
  l1 = l1.substring(0, 15) + (sdOk ? 'S' : '!');
  setLine(0, l1);

  if (!haveData) {
    setLine(1, "Sol: -- %");
    setLine(2, "Eau: -- %");
  } else {
    String s = "Sol: " + soilToState(soilVal) + " " + String(soilVal) + "%";
    if (s.length() > LCD_COLS) s = "Sol:" + soilToState(soilVal) + " " + String(soilVal) + "%";
    setLine(1, s);
    setLine(2, "Eau: " + waterToState(waterVal) + " " + String(waterVal) + "%");
  }
  refreshLine4();
}

// ---------- Scénario des LEDs et niveau d'alerte ----------
void raise(uint8_t s) {
  if (s > severity) severity = s;
}

void evaluate() {
  pattern = P_NONE;
  modeTemp = modeSoil = modeWater = modeOk = M_OFF;
  statusMsg = "";
  severity = 0;

  if (linkLost)       { pattern = P_LINK; severity = 2; return; }
  if (errorMsg != "") { pattern = P_ERR;  severity = 2; return; }
  if (!haveData)      { return; }

  bool irrigating = flowVal >= FLOW_ON;
  bool watering   = irrigating && soilVal < SOIL_MIN && waterVal >= WATER_MIN;
  bool alert = false;

  // Température
  if (tempVal > TEMP_CRIT)     { modeTemp = M_FAST; alert = true; statusMsg = "TEMP CRITIQUE"; raise(2); }
  else if (tempVal > TEMP_MAX) { modeTemp = M_ON;   alert = true; statusMsg = "TEMP ELEVEE";   raise(1); }

  // Sol
  if (watering) {
    modeSoil = M_SLOW; statusMsg = "ARROSAGE";
  } else if (soilVal < SOIL_CRIT) {
    modeSoil = M_FAST; alert = true; statusMsg = "SOL TRES SEC"; raise(2);
  } else if (soilVal < SOIL_MIN) {
    modeSoil = M_ON;   alert = true; statusMsg = "SOL SEC";      raise(1);
  }

  // Réservoir
  if (waterVal < WATER_CRIT)     { modeWater = M_FAST; alert = true; statusMsg = "RESERVOIR VIDE"; raise(2); }
  else if (waterVal < WATER_MIN) { modeWater = M_ON;   alert = true; statusMsg = "EAU BASSE";      raise(1); }

  // Croisements entre capteurs
  if (tempVal > TEMP_MAX && soilVal < SOIL_MIN && !watering) {
    modeTemp = M_FAST; modeSoil = M_FAST; alert = true; statusMsg = "STRESS HYDRIQUE"; raise(2);
  }
  if (soilVal < SOIL_MIN && waterVal < WATER_CRIT) {
    statusMsg = "SECHERESSE";
  }
  if (irrigating && soilVal > SOIL_WETOK) {
    modeSoil = M_FAST; alert = true; statusMsg = "GASPILLAGE"; raise(1);
  }
  if (soilVal > SOIL_SAT && !irrigating) {
    modeSoil = M_SLOW; alert = true; statusMsg = "SOL SATURE"; raise(1);
  }

  // Fuite
  if (flowVal > FLOW_MAX) {
    pattern = P_LEAK; alert = true; statusMsg = "FUITE DEBIT!"; raise(2);
  }

  if (!alert) modeOk = irrigating ? M_SLOW : M_ON;

  // Pompe en marche : message sauf si une alerte critique est déjà affichée
  if (pumpOn && severity < 2) statusMsg = "POMPE ACTIVE";
}

void drive(uint8_t pin, uint8_t mode, bool fast, bool slow) {
  bool v = LOW;
  if (mode == M_ON)   v = HIGH;
  if (mode == M_SLOW) v = slow;
  if (mode == M_FAST) v = fast;
  digitalWrite(pin, v);
}

void ledTick() {
  unsigned long t = millis();
  bool fast = (t / 125) % 2;
  bool slow = (t / 500) % 2;
  bool alt  = (t / 250) % 2;

  switch (pattern) {
    case P_LINK:
      digitalWrite(LED_TEMP, slow);  digitalWrite(LED_SOIL, slow);
      digitalWrite(LED_WATER, slow); digitalWrite(LED_OK, slow);
      break;
    case P_ERR:
      digitalWrite(LED_TEMP, alt);   digitalWrite(LED_SOIL, !alt);
      digitalWrite(LED_WATER, LOW);  digitalWrite(LED_OK, LOW);
      break;
    case P_LEAK:
      digitalWrite(LED_TEMP, alt);   digitalWrite(LED_WATER, !alt);
      digitalWrite(LED_SOIL, LOW);   digitalWrite(LED_OK, LOW);
      break;
    default:
      drive(LED_TEMP,  modeTemp,  fast, slow);
      drive(LED_SOIL,  modeSoil,  fast, slow);
      drive(LED_WATER, modeWater, fast, slow);
      drive(LED_OK,    modeOk,    fast, slow);
  }
}

// ---------- Buzzer (sans delay) ----------
void buzzerTick() {
  unsigned long t = millis();
  bool on = false;

  if (severity == 2) {                       // critique : 3 bips rapides / 2 s
    unsigned long p = t % 2000;
    on = (p < 150) || (p >= 300 && p < 450) || (p >= 600 && p < 750);
  } else if (severity == 1) {                // avertissement : 1 bip / 10 s
    on = (t % 10000) < 100;
  }
  digitalWrite(BUZZER_PIN, on);
}

// ---------- Pompe à eau ----------
void pumpUpdate() {
  unsigned long t = millis();

  // La pompe n'est autorisée que si les données sont fiables et le réservoir non vide
  bool allowed = haveData && !linkLost && errorMsg == "" && waterVal >= WATER_CRIT;
  bool previous = pumpOn;

  if (pumpOn) {
    if (!allowed || soilVal >= SOIL_PUMP_OFF || (t - pumpStart) >= PUMP_MAX_RUN) {
      pumpOn = false;
      pumpStop = t;
    }
  } else {
    if (allowed && soilVal < SOIL_MIN && (t - pumpStop) >= PUMP_REST) {
      pumpOn = true;
      pumpStart = t;
    }
  }

  digitalWrite(PUMP_PIN, pumpOn ? HIGH : LOW);

  if (previous != pumpOn) {
    evaluate();
    refreshLine4();
  }
}

// ---------- Carte SD ----------
bool openSd() {
  if (!SD.begin(SD_CS)) return false;
  if (!SD.exists(LOG_FILE)) {
    File f = SD.open(LOG_FILE, FILE_WRITE);
    if (f) {
      f.println(F("temps_s,temp_C,hum_air_pct,sol_pct,eau_pct,debit_Lmin,etat"));
      f.close();
    }
  }
  return true;
}

void logData() {
  if (!sdOk) sdOk = openSd();
  if (!sdOk) return;

  File f = SD.open(LOG_FILE, FILE_WRITE);
  if (!f) { sdOk = false; return; }

  f.print(millis() / 1000UL); f.print(',');
  f.print(tempVal, 1);        f.print(',');
  f.print(lastHum);           f.print(',');
  f.print(soilVal);           f.print(',');
  f.print(waterVal);          f.print(',');
  f.print(flowVal, 1);        f.print(',');
  if (statusMsg == "") f.println(F("NORMAL"));
  else f.println(statusMsg);
  f.close();
}

void maybeLog() {
  bool changed = (statusMsg != lastLoggedMsg);
  if (changed || millis() - lastLog >= LOG_INTERVAL) {
    lastLog = millis();
    lastLoggedMsg = statusMsg;
    bool before = sdOk;
    logData();
    if (before != sdOk) showValues();
  }
}

// ---------- Réception ----------
void parseLine(String line) {
  line.trim();
  int tIdx = line.indexOf("T:");
  int hIdx = line.indexOf(",H:");
  int sIdx = line.indexOf(",S:");
  int wIdx = line.indexOf(",W:");
  int fIdx = line.indexOf(",F:");

  if (tIdx != -1 && hIdx != -1 && sIdx != -1 && wIdx != -1 && fIdx != -1) {
    lastTemp = line.substring(tIdx + 2, hIdx);
    lastHum  = line.substring(hIdx + 3, sIdx);
    soilVal  = line.substring(sIdx + 3, wIdx).toInt();
    waterVal = line.substring(wIdx + 3, fIdx).toInt();
    flowStr  = line.substring(fIdx + 3);
    tempVal  = lastTemp.toFloat();
    flowVal  = flowStr.toFloat();

    haveData  = true;
    errorMsg  = "";
    linkLost  = false;
    lastFrame = millis();
    evaluate();
    showValues();
    maybeLog();
  } else if (line.startsWith("ERREUR")) {
    errorMsg  = line;
    linkLost  = false;
    lastFrame = millis();
    evaluate();
    showValues();
  }
}

void showSplash() {
  sdOk = openSd();

  lcd.clear();
  setLine(0, "QATRA", true);
  setLine(1, sdOk ? "SD: OK" : "SD: ERREUR", true);
  setLine(2, "Realised by", true);
  setLine(3, "SOLITECH", true);

  digitalWrite(BUZZER_PIN, HIGH);   // bip de démarrage
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);

  delay(2800);
  lcd.clear();
}

void setup() {
  Serial.begin(9600);

  pinMode(LED_TEMP,   OUTPUT);
  pinMode(LED_SOIL,   OUTPUT);
  pinMode(LED_WATER,  OUTPUT);
  pinMode(LED_OK,     OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PUMP_PIN,   OUTPUT);
  digitalWrite(PUMP_PIN, LOW);      // pompe arrêtée au démarrage

  lcd.init();
  lcd.backlight();
  showSplash();

  // Vide le tampon série rempli pendant l'écran de démarrage
  while (Serial.available() > 0) Serial.read();
  rxBuffer = "";

  lastFrame = millis();
  lastLog = millis() - LOG_INTERVAL;   // premier enregistrement immédiat
  pumpStop = millis() - PUMP_REST;     // la pompe peut démarrer dès la première mesure
  evaluate();
  showValues();
}

void loop() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n') {
      parseLine(rxBuffer);
      rxBuffer = "";
    } else if (c != '\r') {
      rxBuffer += c;
    }
  }

  // Chien de garde : liaison avec ARD1
  bool lost = (millis() - lastFrame > LINK_TIMEOUT);
  if (lost != linkLost) {
    linkLost = lost;
    evaluate();
    refreshLine4();
  }

  // Alternance débit / message sur la ligne 4
  if (millis() - lastMsgSwitch >= MSG_INTERVAL) {
    lastMsgSwitch = millis();
    showMsgPhase = !showMsgPhase;
    refreshLine4();
  }

  pumpUpdate();
  ledTick();
  buzzerTick();
}