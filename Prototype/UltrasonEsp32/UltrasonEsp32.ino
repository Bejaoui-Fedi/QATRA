
#define TRIG_PIN 5
#define ECHO_PIN 18

const float TANK_HEIGHT = 9.0;
const float MIN_DISTANCE = 2.0;
const float MAX_DISTANCE = 9.0;

float mesurerDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duree = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duree == 0) {
    return -1;
  }

  return duree * 0.0343 / 2.0;
}

// Calcul de la mediane de 7 mesures
float mesurerMediane() {
  float valeurs[7];
  int compteur = 0;

  for (int i = 0; i < 7; i++) {
    float d = mesurerDistance();

    if (d >= MIN_DISTANCE && d <= MAX_DISTANCE) {
      valeurs[compteur] = d;
      compteur++;
    }

    delay(100);
  }

  if (compteur < 3) {
    return -1;
  }

  // Tri des mesures
  for (int i = 0; i < compteur - 1; i++) {
    for (int j = i + 1; j < compteur; j++) {
      if (valeurs[i] > valeurs[j]) {
        float temp = valeurs[i];
        valeurs[i] = valeurs[j];
        valeurs[j] = temp;
      }
    }
  }

  return valeurs[compteur / 2];
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  Serial.println("===== QATRA =====");
  Serial.println("Mesure intelligente du niveau d'eau");
}

void loop() {
  Serial.println("\n------ QATRA ------");

  float distance = mesurerMediane();

  if (distance == -1) {
    Serial.println("Erreur : mesures insuffisantes");
    Serial.println("--------------------");
    delay(1500);
    return;
  }

  float hauteurEau = TANK_HEIGHT - distance;

  // Limiter les valeurs entre 0 et 9 cm
  hauteurEau = constrain(hauteurEau, 0.0, TANK_HEIGHT);

  float niveau = (hauteurEau / TANK_HEIGHT) * 100.0;

  Serial.print("Distance eau-capteur : ");
  Serial.print(distance, 2);
  Serial.println(" cm");

  Serial.print("Hauteur d'eau : ");
  Serial.print(hauteurEau, 2);
  Serial.println(" cm");

  Serial.print("Niveau du reservoir : ");
  Serial.print(niveau, 1);
  Serial.println(" %");

  if (niveau >= 80) {
    Serial.println("Reservoir presque plein.");
  }
  else if (niveau >= 40) {
    Serial.println("Niveau d'eau normal.");
  }
  else {
    Serial.println("Attention : niveau d'eau faible !");
  }

  Serial.println("--------------------");

  delay(1500);
}
