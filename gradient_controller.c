// FR: Pins pour la LED RGB
// EN: LED RGB pins
#define RED_PIN 25
#define GREEN_PIN 26
#define BLUE_PIN 27

// FR: Communication série
// EN: UART RX and TX definitions
#define RXD2 19
#define TXD2 18

// FR: Structure pour une étape de couleur
// EN: structure for the color stage
struct ColorStep {
  uint8_t r, g, b;
  float duration;  // en secondes (seconds)
};

ColorStep steps[10];  // Max 10 couleurs (max 10 colors)
int stepCount = 0;
int currentStep = 0;
int nextStep = 0;
bool gradientActive = false;

unsigned long lastUpdate = 0;
float progress = 0;  // 0.0 à 1.0 (0.0 to 1.0)

// FR: Couleurs actuelles (interpolées)
// EN: actual colors using interpolation
float currentR = 0, currentG = 0, currentB = 0;

void setRGBColor(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

// FR: Interpolation linéaire entre deux valeurs
// EN: Linear interpolation for between both colors
float interpolate(float a, float b, float t) {
  return a + (b - a) * t;
}

// FR: Fonction d'easing pour une transition plus douce (ease-in-out)
// EN: Easing function for a smoother transition
float easeInOutCubic(float t) {
  return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
}

void updateGradient() {
  if (!gradientActive || stepCount < 2) return;
  
  unsigned long now = millis();
  float deltaTime = (now - lastUpdate) / 1000.0;  // en secondes (seconds)
  lastUpdate = now;
  
  // FR: Durée de la transition actuelle
  // EN: Actual transition duration
  float transitionDuration = steps[currentStep].duration;
  
  // FR: Avancer dans la transition
  // EN: Advance to next transition
  progress += deltaTime / transitionDuration;
  
  // FR: Si on a fini cette transition, passer à la suivante
  // EN: If on final transition, go to next one
  if (progress >= 1.0) {
    progress = 0.0;
    currentStep = nextStep;
    nextStep = (currentStep + 1) % stepCount;
    
    Serial.print("Transition vers couleur ");
    Serial.println(nextStep + 1);
  }
  
  // FR: Appliquer easing pour une transition plus douce
  // EN: Apply more easing for more transition
  float easedProgress = easeInOutCubic(progress);
  
  // FR: Interpoler entre la couleur actuelle et la suivante
  // EN: Interpolate the entire color from current to next
  currentR = interpolate(steps[currentStep].r, steps[nextStep].r, easedProgress);
  currentG = interpolate(steps[currentStep].g, steps[nextStep].g, easedProgress);
  currentB = interpolate(steps[currentStep].b, steps[nextStep].b, easedProgress);
  
  // FR: Appliquer la couleur
  // EN: Apply the color
  setRGBColor((uint8_t)currentR, (uint8_t)currentG, (uint8_t)currentB);
}

// FR: Parser simple pour extraire les valeurs du JSON
// EN: A simple parser to extract values ​​from JSON
int findValue(String json, String key, int startPos) {
  int pos = json.indexOf(key, startPos);
  if (pos == -1) return -1;
  
  pos = json.indexOf(':', pos);
  if (pos == -1) return -1;
  
  // FR: Trouver le début du nombre
  // EN: Find starting number
  pos++;
  while (pos < json.length() && (json[pos] == ' ' || json[pos] == '"')) pos++;
  
  // FR: Extraire le nombre
  // EN: Extract the number
  String numStr = "";
  while (pos < json.length() && (isdigit(json[pos]) || json[pos] == '.')) {
    numStr += json[pos];
    pos++;
  }
  // FR: Retourne la position pour continuer la recherche
  // EN: Returns the position to continue the search
  return pos; 
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
  
  // FR: Compter le nombre d'objets dans steps
  // EN: Compute the number of objects in steps
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
    // FR: Démarrer le gradient
    // EN: Start the gradient
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
  
  // FR: Configuration des pins LED
  // EN: Configure LED pins
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
      // Mode Gradient - recevoir/receive la configuration JSON
      delay(10);  // Attendre que toutes les données arrivent (Wait for all the data to arrive.)
      
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
      // FR: Arrêter le gradient
      // EN: Stop the gradient
      gradientActive = false;
      setRGBColor(0, 0, 0);
      Serial.println("Gradient arrêté\n");
    }
  }
  
  // FR: Mettre à jour le gradient si actif
  // EN: Update if gradient is active
  if (gradientActive) {
    updateGradient();
  }
  
  delay(10);  // Mise à jour ~100 fps
}
