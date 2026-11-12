/*
  Ambulance Signal App - ESP32
  The ESP32 creates its own Wi-Fi and hosts the app.
  Ambulance driver connects the phone, opens http://192.168.4.1 and taps START ALERT.
  Buzzer + green LED turn on in real life. No internet needed.
*/
#include <WiFi.h>
#include <WebServer.h>

const char* AP_SSID = "AmbulanceSignal";
const char* AP_PASS = "12345678";          // change this, min 8 characters

const int GREEN_LED = 25;
const int RED_LED   = 26;
const int BUZZER    = 27;                  // active buzzer
const unsigned long AUTO_OFF_MS = 60000;   // safety: alert stops after 60 s

WebServer server(80);
bool alertOn = false;
unsigned long alertStart = 0;

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#101418">
<title>Ambulance Signal</title>
<style>
*{box-sizing:border-box}
body{margin:0;min-height:100vh;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:28px;background:#101418;color:#f2f4f7;font-family:system-ui,sans-serif;text-align:center;padding:24px;transition:background .3s}
body.on{background:#2a0b0b}
h1{margin:0;font-size:22px;font-weight:600}
#s{margin:0;font-size:16px;color:#aab3bd;max-width:280px;min-height:44px}
body.on #s{color:#ffd0d0}
.wrap{position:relative;width:220px;height:220px}
.ring{position:absolute;inset:0;border-radius:50%;border:3px solid #e5322d;opacity:0}
body.on .ring{animation:p 1.4s infinite}
body.on .r2{animation-delay:.7s}
@keyframes p{0%{transform:scale(1);opacity:.8}100%{transform:scale(1.5);opacity:0}}
#b{position:absolute;inset:0;width:100%;height:100%;border-radius:50%;border:0;background:#e5322d;color:#fff;font-size:22px;font-weight:700;letter-spacing:.5px;box-shadow:0 8px 30px #e5322d66}
body.on #b{background:#1f9d55;box-shadow:0 8px 30px #1f9d5566}
#b:active{transform:scale(.96)}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%;background:#e5322d;margin-right:8px}
body.on .dot{background:#1f9d55}
.st{font-size:14px;color:#aab3bd}
@media (prefers-reduced-motion:reduce){body.on .ring{animation:none;opacity:.5}}
</style></head><body>
<h1>Ambulance Signal</h1>
<div class="wrap"><div class="ring"></div><div class="ring r2"></div><button id="b">START ALERT</button></div>
<p id="s">Connecting to the signal box…</p>
<div class="st"><span class="dot"></span><span id="l">Signal box: red (normal)</span></div>
<script>
var b=document.getElementById('b'),s=document.getElementById('s'),l=document.getElementById('l'),on=false;
function ui(){
  document.body.className=on?'on':'';
  b.textContent=on?'STOP ALERT':'START ALERT';
  s.textContent=on?'Alert active. Buzzer is playing and the light is green.':'Ready. Tap to clear the road ahead.';
  l.textContent=on?'Signal box: green (priority)':'Signal box: red (normal)';
}
function call(p){
  fetch(p).then(function(r){return r.json()}).then(function(j){on=j.on;ui()})
  .catch(function(){s.textContent='Not connected. Join the AmbulanceSignal Wi-Fi and try again.'});
}
b.onclick=function(){if(navigator.vibrate)navigator.vibrate(60);call(on?'/off':'/on')};
call('/state');
setInterval(function(){call('/state')},2000);
</script></body></html>
)rawliteral";

void applyState() {
  digitalWrite(GREEN_LED, alertOn ? HIGH : LOW);
  digitalWrite(RED_LED,   alertOn ? LOW : HIGH);
  if (!alertOn) digitalWrite(BUZZER, LOW);
}

void sendState() {
  server.send(200, "application/json", alertOn ? "{\"on\":true}" : "{\"on\":false}");
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  applyState();

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("Open this on the phone: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/on", []() { alertOn = true; alertStart = millis(); applyState(); sendState(); });
  server.on("/off", []() { alertOn = false; applyState(); sendState(); });
  server.on("/state", sendState);
  server.begin();
}

void loop() {
  server.handleClient();
  if (alertOn) {
    unsigned long now = millis();
    digitalWrite(BUZZER, (now / 400) % 2);     // beep 0.4 s on / 0.4 s off
    if (now - alertStart > AUTO_OFF_MS) {
      alertOn = false;
      applyState();
    }
  }
}
