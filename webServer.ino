#include <WiFi.h>
#include <WebServer.h>

// Configuration WiFi Access Point
const char* ap_ssid = "ESP32-LED-RGB";
const char* ap_password = "12345678";

// UART vers l'autre ESP32
#define RXD2 19
#define TXD2 18

WebServer server(80);

// Structure RGB
struct RGBData {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

RGBData rgbData = {0, 0, 0};

// ----------- PAGE HTML AVEC COLOR PICKER -----------
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Contrôle LED RGB</title>

  <style>
    body {
      font-family: Arial;
      text-align: center;
      padding: 20px;
      background: linear-gradient(135deg, #667eea, #764ba2);
      min-height: 100vh;
      margin: 0;
    }
    .container {
      background: white;
      padding: 25px;
      border-radius: 15px;
      max-width: 420px;
      margin: auto;
      box-shadow: 0 10px 30px rgba(0,0,0,0.3);
    }
    input[type="color"] {
      width: 100%;
      height: 60px;
      border: none;
      margin-bottom: 20px;
      cursor: pointer;
      border-radius: 10px;
    }
    input[type="range"] {
      width: 100%;
    }
    .preview {
      width: 100%;
      height: 100px;
      border-radius: 10px;
      border: 2px solid #ccc;
      margin-bottom: 20px;
    }
    button {
      background: #667eea;
      color: white;
      border: none;
      padding: 12px 25px;
      border-radius: 20px;
      cursor: pointer;
      font-size: 16px;
      margin: 5px;
    }
    button:hover { background: #764ba2; }
  </style>
</head>

<body>
  <div class="container">
    <h1>🎨 Contrôle LED RGB</h1>

    <input type="color" id="colorPicker" value="#000000">

    <div class="preview" id="preview"></div>

    <label>🔴 Rouge: <span id="redValue">0</span></label>
    <input type="range" id="redSlider" min="0" max="255" value="0">

    <label>🟢 Vert: <span id="greenValue">0</span></label>
    <input type="range" id="greenSlider" min="0" max="255" value="0">

    <label>🔵 Bleu: <span id="blueValue">0</span></label>
    <input type="range" id="blueSlider" min="0" max="255" value="0">

    <button onclick="sendColor()">Envoyer Couleur</button>
    <button onclick="turnOff()">Éteindre</button>
  </div>

<script>
const r = document.getElementById('redSlider');
const g = document.getElementById('greenSlider');
const b = document.getElementById('blueSlider');
const cp = document.getElementById('colorPicker');
const preview = document.getElementById('preview');

function rgbToHex(r,g,b) {
  return "#" + 
    Number(r).toString(16).padStart(2,'0') +
    Number(g).toString(16).padStart(2,'0') +
    Number(b).toString(16).padStart(2,'0');
}

function hexToRgb(hex) {
  hex = hex.replace("#","");
  return {
    r: parseInt(hex.substring(0,2),16),
    g: parseInt(hex.substring(2,4),16),
    b: parseInt(hex.substring(4,6),16)
  };
}

function updatePreview() {
  document.getElementById("redValue").textContent = r.value;
  document.getElementById("greenValue").textContent = g.value;
  document.getElementById("blueValue").textContent = b.value;

  preview.style.backgroundColor = `rgb(${r.value},${g.value},${b.value})`;

  cp.value = rgbToHex(r.value, g.value, b.value);
}

r.oninput = updatePreview;
g.oninput = updatePreview;
b.oninput = updatePreview;

cp.oninput = () => {
  const c = hexToRgb(cp.value);
  r.value = c.r;
  g.value = c.g;
  b.value = c.b;
  updatePreview();
};

function sendColor() {
  fetch(`/setColor?r=${r.value}&g=${g.value}&b=${b.value}`);
}

function turnOff() {
  r.value = 0;
  g.value = 0;
  b.value = 0;
  updatePreview();
  fetch('/setColor?r=0&g=0&b=0');
}

updatePreview();
</script>

</body>
</html>
)rawliteral";

// ---------- ROUTES SERVEUR ----------
void handleRoot() {
  server.send(200, "text/html", webpage);
}

void handleSetColor() {
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {

    rgbData.red = server.arg("r").toInt();
    rgbData.green = server.arg("g").toInt();
    rgbData.blue = server.arg("b").toInt();

    Serial2.write('S');
    Serial2.write(rgbData.red);
    Serial2.write(rgbData.green);
    Serial2.write(rgbData.blue);
    Serial2.write('E');

    Serial.printf("Couleur envoyée : R=%d G=%d B=%d\n",
                   rgbData.red, rgbData.green, rgbData.blue);

    server.send(200, "text/plain", "OK");
  }
  else {
    server.send(400, "text/plain", "Paramètres manquants");
  }
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("=== ESP32 Serveur Web ===");

  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial.println("UART Serial2 OK");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("WiFi AP : ");
  Serial.println(ap_ssid);
  Serial.print("IP : ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/setColor", handleSetColor);
  server.begin();

  Serial.println("Serveur Web démarré !");
}

// ---------- LOOP ----------
void loop() {
  server.handleClient();
}

