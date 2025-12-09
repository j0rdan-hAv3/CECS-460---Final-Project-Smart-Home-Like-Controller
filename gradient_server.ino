#include <WiFi.h>
#include <WebServer.h>

// Configuration WiFi Access Point
const char* ap_ssid = "ESP32-LED-RGB";
const char* ap_password = "12345678";

// UART vers l'autre ESP32
#define RXD2 19
#define TXD2 18

WebServer server(80);

// Page HTML avec gradient de couleurs
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Contrôle LED RGB - Gradient</title>
  <style>
    * { box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Arial, sans-serif;
      background: linear-gradient(135deg, #667eea, #764ba2);
      min-height: 100vh;
      margin: 0;
      padding: 20px;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    .container {
      background: white;
      padding: 30px;
      border-radius: 20px;
      max-width: 500px;
      width: 100%;
      box-shadow: 0 15px 40px rgba(0,0,0,0.3);
    }
    h1 { 
      text-align: center;
      color: #333;
      margin-bottom: 25px;
    }
    .color-step {
      background: #f8f9fa;
      padding: 15px;
      border-radius: 10px;
      margin-bottom: 15px;
      border: 2px solid #e9ecef;
    }
    .color-step.active {
      border-color: #667eea;
      background: #f0f4ff;
    }
    .step-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 10px;
    }
    .step-number {
      font-weight: bold;
      color: #667eea;
      font-size: 18px;
    }
    .color-preview {
      width: 50px;
      height: 50px;
      border-radius: 8px;
      border: 2px solid #ddd;
      cursor: pointer;
    }
    .controls {
      display: flex;
      gap: 10px;
      align-items: center;
    }
    input[type="color"] {
      width: 60px;
      height: 40px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
    }
    .duration-control {
      flex: 1;
      display: flex;
      flex-direction: column;
      gap: 5px;
    }
    .duration-control label {
      font-size: 12px;
      color: #666;
      font-weight: 500;
    }
    input[type="range"] {
      width: 100%;
      height: 6px;
      border-radius: 5px;
      outline: none;
      -webkit-appearance: none;
      background: #ddd;
    }
    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 18px;
      height: 18px;
      border-radius: 50%;
      background: #667eea;
      cursor: pointer;
    }
    .duration-value {
      font-weight: bold;
      color: #667eea;
      font-size: 14px;
    }
    .button-group {
      display: flex;
      gap: 10px;
      margin-top: 20px;
    }
    button {
      flex: 1;
      background: #667eea;
      color: white;
      border: none;
      padding: 14px;
      border-radius: 10px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s;
    }
    button:hover {
      background: #5568d3;
      transform: translateY(-2px);
      box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
    }
    button.secondary {
      background: #6c757d;
    }
    button.secondary:hover {
      background: #5a6268;
    }
    button.danger {
      background: #dc3545;
    }
    button.danger:hover {
      background: #c82333;
    }
    .add-step {
      background: #28a745;
      margin-bottom: 15px;
    }
    .add-step:hover {
      background: #218838;
    }
    .gradient-preview {
      width: 100%;
      height: 80px;
      border-radius: 10px;
      margin: 20px 0;
      border: 2px solid #ddd;
    }
    .status {
      text-align: center;
      padding: 10px;
      border-radius: 8px;
      margin-top: 15px;
      display: none;
      font-weight: 500;
    }
    .status.show {
      display: block;
      background: #d4edda;
      color: #155724;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎨 Contrôle LED RGB - Gradient</h1>
    
    <div class="gradient-preview" id="gradientPreview"></div>
    
    <button class="add-step" onclick="addStep()">➕ Ajouter une couleur</button>
    
    <div id="stepsContainer"></div>
    
    <div class="button-group">
      <button onclick="startGradient()">▶️ Démarrer</button>
      <button class="secondary" onclick="stopGradient()">⏸️ Arrêter</button>
      <button class="danger" onclick="clearAll()">🗑️ Effacer tout</button>
    </div>
    
    <div class="status" id="status"></div>
  </div>

<script>
let steps = [
  { color: '#ff0000', duration: 2 },
  { color: '#00ff00', duration: 2 },
  { color: '#0000ff', duration: 2 }
];
let isRunning = false;
let currentStepIndex = 0;

function hexToRgb(hex) {
  hex = hex.replace('#', '');
  return {
    r: parseInt(hex.substring(0, 2), 16),
    g: parseInt(hex.substring(2, 4), 16),
    b: parseInt(hex.substring(4, 6), 16)
  };
}

function updateGradientPreview() {
  const colors = steps.map(s => s.color).join(', ');
  document.getElementById('gradientPreview').style.background = 
    `linear-gradient(to right, ${colors})`;
}

function renderSteps() {
  const container = document.getElementById('stepsContainer');
  container.innerHTML = '';
  
  steps.forEach((step, index) => {
    const div = document.createElement('div');
    div.className = 'color-step';
    if (isRunning && index === currentStepIndex) {
      div.className += ' active';
    }
    
    div.innerHTML = `
      <div class="step-header">
        <span class="step-number">Couleur ${index + 1}</span>
        <div style="display: flex; gap: 10px; align-items: center;">
          <div class="color-preview" style="background: ${step.color}"></div>
          ${steps.length > 2 ? `<button onclick="removeStep(${index})" style="padding: 5px 10px; font-size: 12px; background: #dc3545;">❌</button>` : ''}
        </div>
      </div>
      <div class="controls">
        <input type="color" value="${step.color}" onchange="updateColor(${index}, this.value)">
        <div class="duration-control">
          <label>Durée: <span class="duration-value">${step.duration}s</span></label>
          <input type="range" min="0.5" max="10" step="0.5" value="${step.duration}" 
                 oninput="updateDuration(${index}, this.value)">
        </div>
      </div>
    `;
    
    container.appendChild(div);
  });
  
  updateGradientPreview();
}

function updateColor(index, color) {
  steps[index].color = color;
  renderSteps();
}

function updateDuration(index, duration) {
  steps[index].duration = parseFloat(duration);
  renderSteps();
}

function addStep() {
  const lastColor = steps[steps.length - 1].color;
  steps.push({ color: lastColor, duration: 2 });
  renderSteps();
  showStatus('Couleur ajoutée !');
}

function removeStep(index) {
  if (steps.length > 2) {
    steps.splice(index, 1);
    renderSteps();
    showStatus('Couleur supprimée');
  }
}

function clearAll() {
  steps = [
    { color: '#000000', duration: 2 },
    { color: '#000000', duration: 2 }
  ];
  stopGradient();
  renderSteps();
  showStatus('Tout effacé');
}

function showStatus(message) {
  const status = document.getElementById('status');
  status.textContent = message;
  status.className = 'status show';
  setTimeout(() => status.className = 'status', 2000);
}

async function startGradient() {
  if (isRunning) return;
  
  isRunning = true;
  showStatus('Gradient démarré !');
  
  // Envoyer la configuration au serveur
  const config = {
    steps: steps.map(s => {
      const rgb = hexToRgb(s.color);
      return {
        r: rgb.r,
        g: rgb.g,
        b: rgb.b,
        duration: s.duration
      };
    })
  };
  
  await fetch('/startGradient', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(config)
  });
  
  animateSteps();
}

function animateSteps() {
  if (!isRunning) return;
  
  currentStepIndex = (currentStepIndex + 1) % steps.length;
  renderSteps();
  
  const nextDelay = steps[currentStepIndex].duration * 1000;
  setTimeout(animateSteps, nextDelay);
}

async function stopGradient() {
  isRunning = false;
  currentStepIndex = 0;
  renderSteps();
  showStatus('Gradient arrêté');
  
  await fetch('/stopGradient');
}

renderSteps();
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", webpage);
}

void handleStartGradient() {
  if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    
    // Envoyer la commande START au client
    Serial2.write('G');  // Mode Gradient
    Serial2.write((uint8_t)(body.length() >> 8));
    Serial2.write((uint8_t)(body.length() & 0xFF));
    Serial2.print(body);
    
    Serial.println("Gradient démarré:");
    Serial.println(body);
    
    server.send(200, "text/plain", "OK");
  }
}

void handleStopGradient() {
  // Envoyer la commande STOP
  Serial2.write('X');  // Stop
  
  Serial.println("Gradient arrêté");
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("\n=== ESP32 Serveur Web - Mode Gradient ===");

  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial.println("✓ UART Serial2 initialisé");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("WiFi AP: ");
  Serial.println(ap_ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/startGradient", handleStartGradient);
  server.on("/stopGradient", handleStopGradient);
  server.begin();

  Serial.println("✓ Serveur web démarré !");
  Serial.println("Connectez-vous à http://192.168.4.1\n");
}

void loop() {
  server.handleClient();
}
