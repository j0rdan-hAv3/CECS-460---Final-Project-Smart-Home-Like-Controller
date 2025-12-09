// Pins pour la LED RGB
#define RED_PIN 25
#define GREEN_PIN 26
#define BLUE_PIN 27

// Communication série
#define RXD2 19
#define TXD2 18

// Structure pour une étape de couleur
struct ColorStep {
  uint8_t r, g, b;
  float duration;  // en secondes
};

ColorStep steps[10];  // Max 10 couleurs
int stepCount = 0;
int currentStep = 0;
int nextStep = 0;
bool gradientActive = false;

unsigned long lastUpdate = 0;
float progress = 0;  // 0.0 à 1.0

// Couleurs actuelles (interpolées)
float currentR = 0, currentG = 0, currentB = 0;

void setRGBColor(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

// Interpolation linéaire entre deux valeurs
float interpolate(float a, float b, float t) {
  return a + (b - a) * t;
}

// Fonction d'easing pour une transition plus douce (ease-in-out)
float easeInOutCubic(float t) {
  return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
}

void updateGradient() {
  if (!gradientActive || stepCount < 2) return;
  
  unsigned long now = millis();
  float deltaTime = (now - lastUpdate) / 1000.0;  // en secondes
  lastUpdate = now;
  
  // Durée de la transition actuelle
  float transitionDuration = steps[currentStep].duration;
  
  // Avancer dans la transition
  progress += deltaTime / transitionDuration;
  
  // Si on a fini cette transition, passer à la suivante
  if (progress >= 1.0) {
    progress = 0.0;
    currentStep = nextStep;
    nextStep = (currentStep + 1) % stepCount;
    
    Serial.print("Transition vers couleur ");
    Serial.println(nextStep + 1);
  }
  
  // Appliquer easing pour une transition plus douce
  float easedProgress = easeInOutCubic(progress);
  
  // Interpoler entre la couleur actuelle et la suivante
  currentR = interpolate(steps[currentStep].r, steps[nextStep].r, easedProgress);
  currentG = interpolate(steps[currentStep].g, steps[nextStep].g, easedProgress);
  currentB = interpolate(steps[currentStep].b, steps[nextStep].b, easedProgress);
  
  // Appliquer la couleur
  setRGBColor((uint8_t)currentR, (uint8_t)currentG, (uint8_t)currentB);
}

// Parser simple pour extraire les valeurs du JSON
int findValue(String json, String key, int startPos) {
  int pos = json.indexOf(key, startPos);
  if (pos == -1) return -1;
  
  pos = json.indexOf(':', pos);
  if (pos == -1) return -1;
  
  // Trouver le début du nombre
  pos++;
  while (pos < json.length() && (json[pos] == ' ' || json[pos] == '"')) pos++;
  
  // Extraire le nombre
  String numStr = "";
  while (pos < json.length() && (isdigit(json[pos]) || json[pos] == '.')) {
    numStr += json[pos];
    pos++;
  }
  
  return pos;  // Retourne la position pour continuer la recherche
}

float extractFloat(String json, String key, int startPos) {
  int pos = json.indexOf(key, startPos);
  if (pos == -1) return 0;
  
  pos = json.indexOf(':', pos) + 1;
  while (pos < json.length() && (json[pos] == ' ' || json[pos] == '"')) pos++;
  
  String numStr = "";
  while (pos < json.length() && (isdigit(json[pos]) || json[pos] == '.')) {
    numStr += json[pos];
    pos++;
  }
  
  return numStr.toFloat();
}

int extractInt(String json, String key, int startPos) {
  int pos = json.indexOf(key, startPos);
  if (pos == -1) return 0;
  
  pos = json.indexOf(':', pos) + 1;
  while (pos < json.length() && json[pos] == ' ') pos++;
  
  String numStr = "";
  while (pos < json.length() && isdigit(json[pos])) {
    numStr += json[pos];
    pos++;
  }
  
  return numStr.toInt();
}

void parseGradientConfig(String json) {
  Serial.println("Parsing configuration:");
  Serial.println(json);
  
  // Compter le nombre d'objets dans steps
  stepCount = 0;
  int pos = 0;
  while (pos < json.length() && stepCount < 10) {
    pos = json.indexOf("\"r\"", pos);
    if (pos == -1) break;
    
    steps[stepCount].r = extractInt(json, "\"r\"", pos);
    steps[stepCount].g = extractInt(json, "\"g\"", pos);
    steps[stepCount].b = extractInt(json, "\"b\"", pos);
    steps[stepCount].duration = extractFloat(json, "\"duration\"", pos);
    
    Serial.print("  Couleur ");
    Serial.print(stepCount + 1);
    Serial.print(": R=");
    Serial.print(steps[stepCount].r);
    Serial.print(" G=");
    Serial.print(steps[stepCount].g);
    Serial.print(" B=");
    Serial.print(steps[stepCount].b);
    Serial.print(" Durée=");
    Serial.print(steps[stepCount].duration);
    Serial.println("s");
    
    stepCount++;
    pos++;
  }
  
  Serial.print("✓ ");
  Serial.print(stepCount);
  Serial.println(" couleurs configurées");
  
  if (stepCount >= 2) {
    // Démarrer le gradient
    currentStep = 0;
    nextStep = 1;
    progress = 0;
    lastUpdate = millis();
    gradientActive = true;
    
    Serial.println("✓ Gradient démarré !\n");
  } else {
    Serial.println("✗ Besoin d'au moins 2 couleurs\n");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 Client LED RGB - Mode Gradient ===");
  
  // Configuration des pins LED
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  
  // Configuration PWM
  ledcAttach(RED_PIN, 5000, 8);
  ledcAttach(GREEN_PIN, 5000, 8);
  ledcAttach(BLUE_PIN, 5000, 8);
  
  setRGBColor(0, 0, 0);
  
  Serial.println("✓ LED RGB configurée");
  
  // Initialiser UART Serial2
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial.println("✓ UART Serial2 initialisé");
  
  Serial.println("\n=== En attente de commandes ===\n");
}

void loop() {
  // Vérifier les commandes UART
  if (Serial2.available()) {
    char cmd = Serial2.read();
    
    if (cmd == 'G') {
      // Mode Gradient - recevoir la configuration JSON
      delay(10);  // Attendre que toutes les données arrivent
      
      if (Serial2.available() >= 2) {
        int len = (Serial2.read() << 8) | Serial2.read();
        
        Serial.print("Réception de ");
        Serial.print(len);
        Serial.println(" bytes");
        
        String json = "";
        unsigned long timeout = millis() + 2000;
        while (json.length() < len && millis() < timeout) {
          if (Serial2.available()) {
            json += (char)Serial2.read();
          }
          delay(1);
        }
        
        if (json.length() == len) {
          parseGradientConfig(json);
        } else {
          Serial.print("✗ Timeout - reçu seulement ");
          Serial.print(json.length());
          Serial.println(" bytes");
        }
      }
    }
    else if (cmd == 'X') {
      // Arrêter le gradient
      gradientActive = false;
      setRGBColor(0, 0, 0);
      Serial.println("Gradient arrêté\n");
    }
  }
  
  // Mettre à jour le gradient si actif
  if (gradientActive) {
    updateGradient();
  }
  
  delay(10);  // Mise à jour ~100 fps
}
