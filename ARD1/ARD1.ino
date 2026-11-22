#include <Wire.h>

#define SHT21_ADDR 0x40
#define CMD_TEMP   0xE3
#define CMD_HUM    0xE5

#define SOIL_PIN   A0
#define WATER_PIN  A1
#define FLOW_PIN   2        // YF-S201 : signal jaune (INT0)

// Calibration du capteur de sol (inversée pour Proteus)
const int SOIL_DRY = 0;
const int SOIL_WET = 1023;

// Calibration du capteur d'eau
const int WATER_EMPTY = 0;
const int WATER_FULL  = 1023;

// YF-S201 : fréquence (Hz) = 7.5 x débit (L/min)
const float FLOW_K = 7.5;

unsigned long lastSend = 0;
const unsigned long INTERVAL = 2000;

volatile unsigned long pulseCount = 0;
unsigned long lastFlowTime = 0;

void pulseISR() {
  pulseCount++;
}

bool sht21ReadRaw(uint8_t cmd, uint16_t &raw) {
  Wire.beginTransmission(SHT21_ADDR);
  Wire.write(cmd);
  if (Wire.endTransmission() != 0) return false;

  delay(100);

  if (Wire.requestFrom(SHT21_ADDR, 3) != 3) return false;

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  Wire.read();   // CRC ignoré

  raw = ((uint16_t)msb << 8) | lsb;
  raw &= 0xFFFC;
  return true;
}

int readAnalogAvg(int pin) {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(pin);
    delay(5);
  }
  return (int)(sum / 10);
}

int readSoilPercent() {
  int pct = map(readAnalogAvg(SOIL_PIN), SOIL_DRY, SOIL_WET, 0, 100);
  return constrain(pct, 0, 100);
}

int readWaterPercent() {
  int pct = map(readAnalogAvg(WATER_PIN), WATER_EMPTY, WATER_FULL, 0, 100);
  return constrain(pct, 0, 100);
}

// Débit moyen (L/min) depuis la dernière lecture
float readFlow() {
  noInterrupts();
  unsigned long p = pulseCount;
  pulseCount = 0;
  interrupts();

  unsigned long now = millis();
  float dt = (now - lastFlowTime) / 1000.0;
  lastFlowTime = now;

  if (dt < 0.2) return 0.0;
  float hz = (float)p / dt;
  return hz / FLOW_K;
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseISR, RISING);

  delay(500);
  lastFlowTime = millis();
  lastSend = millis() - INTERVAL;   // première mesure immédiate
}

void loop() {
  if (millis() - lastSend >= INTERVAL) {
    lastSend = millis();

    float flow = readFlow();   // lu avant les autres capteurs

    uint16_t rawT, rawH;

    if (sht21ReadRaw(CMD_TEMP, rawT) && sht21ReadRaw(CMD_HUM, rawH)) {
      float temp = -46.85 + 175.72 * (float)rawT / 65536.0;
      float hum  = -6.0   + 125.0  * (float)rawH / 65536.0;
      int soil   = readSoilPercent();
      int water  = readWaterPercent();

      Serial.print("T:");
      Serial.print(temp, 1);
      Serial.print(",H:");
      Serial.print(hum, 1);
      Serial.print(",S:");
      Serial.print(soil);
      Serial.print(",W:");
      Serial.print(water);
      Serial.print(",F:");
      Serial.println(flow, 1);
    } else {
      Serial.println("ERREUR SHT21");
    }
  }
}