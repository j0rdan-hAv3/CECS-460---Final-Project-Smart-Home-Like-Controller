// FR: Pins pour la LED RGB 
// EN: RGB pins
#define RED_PIN 25
#define GREEN_PIN 26
#define BLUE_PIN 27

// FR: Communication série
#define RXD2 19
#define TXD2 18

// FR: Données RGB reçues 
// EN: RGB data start value
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;

void setRGBColor(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
  
  Serial.print("LED: R=");
  Serial.print(r);
  Serial.print(" G=");
  Serial.print(g);
  Serial.print(" B=");
  Serial.println(b);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 Client LED RGB (UART Slave) ===");
  
  // Configuration des pins LED
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  // FR: Tout éteindre
  // EN: turn off on low
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
  Serial.println("✓ UART Serial2 initialisé (RX=19, TX=18)");
  
  Serial.println("\n=== En attente de commandes UART ===\n");
}

void loop() {
  // FR: Vérifier si des données sont disponibles
  // EN: verify if the data are available
  if (Serial2.available() >= 5) {
    // FR: Lire le byte de début
    // EN: Read begin byte
    if (Serial2.read() == 'S') {
      red = Serial2.read();
      green = Serial2.read();
      blue = Serial2.read();
      
      // FR: Vérifier le byte de fin 
      // EN: Verify finishing byte.
      if (Serial2.read() == 'E') {
        Serial.println("Commande reçue:");
        Serial.print("  Rouge: "); Serial.println(red);
        Serial.print("  Vert: "); Serial.println(green);
        Serial.print("  Bleu: "); Serial.println(blue);
        
        setRGBColor(red, green, blue);
      } else {
        Serial.println("Erreur: byte de fin invalide");
      }
    }
  }
  
  delay(0);
