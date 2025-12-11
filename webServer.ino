#include <WiFi.h>
#include <WebServer.h>

// Configuration WiFi
const char* ap_ssid = "ESP32-LED-RGB";
const char* ap_password = "12345678";

// UART
#define RXD2 19
#define TXD2 18

WebServer server(80);

// HTML minifié et compressé
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>LED RGB</title><style>body{font-family:Arial;text-align:center;padding:20px;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;margin:0}.container{background:#fff;padding:25px;border-radius:15px;max-width:420px;margin:auto;box-shadow:0 10px 30px rgba(0,0,0,.3)}input[type=color]{width:100%;height:60px;border:none;margin-bottom:20px;cursor:pointer;border-radius:10px}input[type=range]{width:100%}.preview{width:100%;height:100px;border-radius:10px;border:2px solid #ccc;margin-bottom:20px}button{background:#667eea;color:#fff;border:none;padding:12px 25px;border-radius:20px;cursor:pointer;font-size:16px;margin:5px}button:hover{background:#764ba2}</style></head><body><div class="container"><h1>🎨 LED RGB</h1><input type="color" id="c" value="#000000"><div class="preview" id="p"></div><label>🔴 Rouge: <span id="rv">0</span></label><input type="range" id="r" min="0" max="255" value="0"><label>🟢 Vert: <span id="gv">0</span></label><input type="range" id="g" min="0" max="255" value="0"><label>🔵 Bleu: <span id="bv">0</span></label><input type="range" id="b" min="0" max="255" value="0"><button onclick="s()">Envoyer</button><button onclick="o()">Éteindre</button></div><script>const r=document.getElementById('r'),g=document.getElementById('g'),b=document.getElementById('b'),c=document.getElementById('c'),p=document.getElementById('p');function h2r(h){h=h.replace("#","");return{r:parseInt(h.substring(0,2),16),g:parseInt(h.substring(2,4),16),b:parseInt(h.substring(4,6),16)}}function u(){document.getElementById("rv").textContent=r.value;document.getElementById("gv").textContent=g.value;document.getElementById("bv").textContent=b.value;p.style.backgroundColor='rgb('+r.value+','+g.value+','+b.value+')';c.value="#"+Number(r.value).toString(16).padStart(2,'0')+Number(g.value).toString(16).padStart(2,'0')+Number(b.value).toString(16).padStart(2,'0')}r.oninput=g.oninput=b.oninput=u;c.oninput=()=>{const x=h2r(c.value);r.value=x.r;g.value=x.g;b.value=x.b;u()};function s(){fetch('/s?r='+r.value+'&g='+g.value+'&b='+b.value)}function o(){r.value=g.value=b.value=0;u();fetch('/s?r=0&g=0&b=0')}u()</script></body></html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", webpage);
}

void handleSetColor() {
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
    uint8_t rv = server.arg("r").toInt();
    uint8_t gv = server.arg("g").toInt();
    uint8_t bv = server.arg("b").toInt();

    // ENVOI UART PRIORITAIRE - AVANT la réponse HTTP
    uint8_t data[] = {'S', rv, gv, bv, 'E'};
    Serial2.write(data, 5);
    Serial2.flush(); // Force l'envoi immédiat

    // Réponse HTTP minimale
    server.send(200, "text/plain", "");
  } else {
    server.send(400, "text/plain", "");
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial2.setRxBufferSize(256); // Buffer minimal
  Serial2.setTxBufferSize(256);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  // Désactiver le sleep mode WiFi pour réactivité maximale
  WiFi.setSleep(false);

  server.on("/", handleRoot);
  server.on("/s", handleSetColor);
  server.begin();

  Serial.println("Ready");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
  yield(); // Permet au WiFi de respirer sans bloquer
}
