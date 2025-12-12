#include "WifiModule.h"
#include "WatchWinderApp.h"

// Optional secrets file. Define WIFI_SSID_VALUE and WIFI_PASS_VALUE there.
#if defined(__has_include)
#if __has_include("WifiSecrets.h")
#include "WifiSecrets.h"
#endif
#endif

#ifndef WIFI_SSID_VALUE
#define WIFI_SSID_VALUE ""
#endif
#ifndef WIFI_PASS_VALUE
#define WIFI_PASS_VALUE ""
#endif

#if WIFI_MODULE_ENABLED

static const char DASHBOARD_HTML[] =
R"HTML(<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Watch Winder</title><style>
:root{--bg:#0d1b2a;--card:#10243a;--accent:#4ad6b8;--text:#e6f1ff;--muted:#9fb2c8;--warn:#f2a65a}
*{box-sizing:border-box;font-family:"Segoe UI",system-ui,sans-serif}
body{margin:0;padding:24px;background:radial-gradient(120% 120% at 20% 20%,#16324f 0%,#0d1b2a 45%,#0b1624 100%);color:var(--text)}
h1,h2,h3{margin:0;font-weight:700}
a{color:var(--accent)}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:16px;margin-top:18px}
.card{background:var(--card);border:1px solid rgba(255,255,255,0.08);border-radius:14px;padding:16px;box-shadow:0 20px 40px rgba(0,0,0,0.25);backdrop-filter:blur(6px)}
.pill{display:inline-block;padding:4px 10px;border-radius:999px;font-size:12px;font-weight:600;background:rgba(255,255,255,0.08);color:var(--accent);margin-right:6px}
.row{display:flex;gap:10px;flex-wrap:wrap;margin:10px 0}
label{display:block;font-size:13px;color:var(--muted);margin-bottom:4px}
input,select,button{width:100%;padding:10px;border-radius:10px;border:1px solid rgba(255,255,255,0.12);background:rgba(255,255,255,0.03);color:var(--text);font-size:14px}
button{cursor:pointer;border:none;background:linear-gradient(120deg,#4ad6b8,#4a9ad6);font-weight:700;transition:transform .1s ease,box-shadow .1s ease}
button:hover{transform:translateY(-1px);box-shadow:0 10px 20px rgba(74,214,184,0.35)}
button.danger{background:linear-gradient(120deg,#ef476f,#f78c6b)}
.stat{font-size:22px;font-weight:700}
.muted{color:var(--muted);font-size:13px}
.badge-warn{background:rgba(242,166,90,0.18);color:var(--warn)}
.hero{display:flex;flex-direction:column;gap:8px}
</style></head><body>
<div class="hero"><h1>Watch Winder</h1><div class="muted" id="ipLine">Connecting…</div><div id="modeLine" class="pill"></div><button style="width:140px" onclick="location.reload()">Refresh</button></div>
<div class="grid">
  <div class="card"><h3>Status</h3><div class="row"><div><div class="muted">WiFi</div><div class="stat" id="wifiState">-</div></div><div><div class="muted">Active</div><div class="stat" id="activePreset">-</div></div><div><div class="muted">Selected</div><div class="stat" id="selectedPreset">-</div></div></div><div class="muted" id="scheduleLine"></div><div class="muted">Last preset: <span id="lastPreset">-</span></div><div class="muted" id="uptimeLine"></div><div class="muted" id="wifiCounters"></div></div>
  <div class="card"><div class="row" style="justify-content:space-between;align-items:center;"><h3 style="margin:0">Controls</h3><button onclick="location.reload()">Refresh</button></div><div class="row" style="flex-direction:column;gap:8px;">
    <button onclick="startPreset(1)">Preset 1: Motor1 CW 100</button>
    <button onclick="startPreset(2)">Preset 2: Motor2 CW 100</button>
    <button onclick="startPreset(3)">Preset 3: Both CW 100</button>
    <button onclick="startPreset(4)">Preset 4: Motor1 CCW 50</button>
    <button onclick="startPreset(5)">Preset 5: Motor2 CCW 50</button>
    <button onclick="startPreset(6)">Preset 6: Both CCW 50</button>
    <button onclick="startPreset(7)">Preset 7: Both alt CW/CCW duty</button>
    <button onclick="startPreset(8)">Preset 8: M1 CW / M2 CCW duty</button>
  </div>
  <div class="row"><button class="danger" onclick="stopAll()">Stop</button></div>
  <div class="row"><div style="flex:1"><label>Schedule preset (seconds)</label><input id="schedDelay" type="number" min="5" max="86400" value="60"></div><div style="flex:1;align-self:flex-end"><button onclick="schedulePreset()">Schedule Last Selected</button></div><div style="flex:1;align-self:flex-end"><button class="danger" onclick="cancelSchedule()">Cancel schedule</button></div></div></div>
  <div class="card"><h3>Manual drive</h3><div class="row"><div style="flex:1"><label>Motors</label><select id="mMotors"><option value="both">Both</option><option value="m1">Motor 1</option><option value="m2">Motor 2</option></select></div><div style="flex:1"><label>Dir M1</label><select id="mDir1"><option value="cw">CW</option><option value="ccw">CCW</option></select></div><div style="flex:1"><label>Dir M2</label><select id="mDir2"><option value="cw">CW</option><option value="ccw">CCW</option></select></div><div style="flex:1"><label>Speed</label><select id="mSpeed"><option value="normal">Normal</option><option value="slow">Slow</option><option value="fast">Fast</option></select></div></div><div class="row"><button onclick="manualStart()">Start manual</button><button class="danger" onclick="manualStop()">Stop manual</button></div><div class="muted" id="manualLine"></div></div>
  <div class="card"><h3>Metrics</h3><div class="row"><div style="flex:1"><div class="muted">Motor 1</div><div>Turns CW: <span id="m1cw">0</span></div><div>Turns CCW: <span id="m1ccw">0</span></div><div>TPM: <span id="m1tpm">0</span></div><div>TPD: <span id="m1tpd">0</span></div><div>Starts: <span id="m1starts">0</span></div><div>Run sec: <span id="m1run">0</span></div></div><div style="flex:1"><div class="muted">Motor 2</div><div>Turns CW: <span id="m2cw">0</span></div><div>Turns CCW: <span id="m2ccw">0</span></div><div>TPM: <span id="m2tpm">0</span></div><div>TPD: <span id="m2tpd">0</span></div><div>Starts: <span id="m2starts">0</span></div><div>Run sec: <span id="m2run">0</span></div></div></div></div>
</div>
<script>
const $=id=>document.getElementById(id);
async function refresh(){
  try{
    const res=await fetch('/status'); const s=await res.json();
    $('wifiState').textContent=s.connected?'Online':'Offline';
    $('ipLine').textContent=s.connected?`IP: ${s.ip}`:'Disconnected';
    $('uptimeLine').textContent=`Uptime: ${s.uptimeSec}s`;
    $('wifiCounters').textContent=`Requests: ${s.wifiCounters.requests}, Reconnects: ${s.wifiCounters.reconnects}`;
    $('activePreset').textContent=s.mode==='preset'?`Preset #${s.activePreset.id} (${s.activePreset.label||''})`: (s.mode==='manual'?'Manual':'Idle');
    $('selectedPreset').textContent=s.selectedPreset.id?`#${s.selectedPreset.id} (${s.selectedPreset.label||''})`:'None';
    $('lastPreset').textContent=s.lastPreset||'0';
    $('modeLine').textContent=`Mode: ${s.mode}`;
    $('scheduleLine').textContent=s.schedule.armed?`Scheduled preset #${s.schedule.presetId} in ${s.schedule.remaining}s (executed ${s.schedule.executed})`:`No schedule (executed ${s.schedule.executed})`;
    $('manualLine').textContent=s.manual.active?`Manual ${s.manual.motors} dir1=${s.manual.dir1} dir2=${s.manual.dir2} speed=${s.manual.speed}`:'';
    $('m1cw').textContent=s.metrics.m1.cw.toFixed(2);
    $('m1ccw').textContent=s.metrics.m1.ccw.toFixed(2);
    $('m1tpm').textContent=s.metrics.m1.tpm.toFixed(3);
    $('m1tpd').textContent=s.metrics.m1.tpd.toFixed(2);
    $('m1starts').textContent=s.metrics.m1.starts;
    $('m1run').textContent=s.metrics.m1.runSec;
    $('m2cw').textContent=s.metrics.m2.cw.toFixed(2);
    $('m2ccw').textContent=s.metrics.m2.ccw.toFixed(2);
    $('m2tpm').textContent=s.metrics.m2.tpm.toFixed(3);
    $('m2tpd').textContent=s.metrics.m2.tpd.toFixed(2);
    $('m2starts').textContent=s.metrics.m2.starts;
    $('m2run').textContent=s.metrics.m2.runSec;
  }catch(e){console.log(e);}
}
function startPreset(id){localStorage.setItem('lastPresetId',id);fetch(`/start?id=${id}`).then(()=>window.location.reload());}
function stopAll(){fetch('/stop').then(()=>window.location.reload());}
function schedulePreset(){const id=localStorage.getItem('lastPresetId')||1;const delay=$('schedDelay').value;fetch(`/schedule?id=${id}&delay=${delay}`).then(()=>window.location.reload());}
function cancelSchedule(){fetch('/schedule/cancel').then(()=>window.location.reload());}
function manualStart(){const m=$('mMotors').value,d1=$('mDir1').value,d2=$('mDir2').value,s=$('mSpeed').value;fetch(`/manual?motors=${m}&dir1=${d1}&dir2=${d2}&speed=${s}`).then(()=>window.location.reload());}
function manualStop(){fetch('/manual/stop').then(()=>window.location.reload());}
refresh();
</script></body></html>)HTML";

static const char WIFI_SSID[] = WIFI_SSID_VALUE;
static const char WIFI_PASS[] = WIFI_PASS_VALUE;

static bool applyStaticConfig() {
#if defined(WIFI_STATIC_IP)
  IPAddress ip, gw, mask, dns;
  bool ok = ip.fromString(WIFI_STATIC_IP);
  bool gwOk = false, maskOk = false, dnsOk = false;
#if defined(WIFI_STATIC_GATEWAY)
  gwOk = gw.fromString(WIFI_STATIC_GATEWAY);
#endif
#if defined(WIFI_STATIC_SUBNET)
  maskOk = mask.fromString(WIFI_STATIC_SUBNET);
#endif
#if defined(WIFI_STATIC_DNS)
  dnsOk = dns.fromString(WIFI_STATIC_DNS);
#endif
  if (ok) {
    if (!gwOk) gw = IPAddress(ip[0], ip[1], ip[2], 1);
    if (!maskOk) mask = IPAddress(255, 255, 255, 0);
    if (!dnsOk) dns = gw;
    // Order for WiFiS3: config(local_ip, dns, gateway, subnet)
    WiFi.config(ip, dns, gw, mask);
    return true;
  }
#endif
  return false;
}

bool WifiModule::begin(WatchWinderApp *appPtr) {
  app = appPtr;
  if (strlen(WIFI_SSID) == 0) {
    Serial.println(F("[WiFi] Skipping WiFi (no credentials set)"));
    enabledFlag = false;
    connectedFlag = false;
    return false;
  }

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("[WiFi] No WiFi shield detected; disabling WiFi"));
    enabledFlag = false;
    connectedFlag = false;
    return false;
  }

  enabledFlag = true;
  Serial.print(F("[WiFi] Connecting to "));
  Serial.println(WIFI_SSID);
  applyStaticConfig();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long startWait = millis();
  const unsigned long CONNECT_TIMEOUT_MS = 12000;
  auto status = WiFi.status();
  while (status != WL_CONNECTED && (millis() - startWait) < CONNECT_TIMEOUT_MS) {
    delay(200);
    status = WiFi.status();
  }
  lastConnectAttempt = millis();
  connectedFlag = status == WL_CONNECTED && WiFi.localIP() != IPAddress(0,0,0,0);
  if (connectedFlag) {
    IPAddress ip = WiFi.localIP();
    server.begin();
    Serial.print(F("[WiFi] Connected, IP="));
    Serial.println(ip);
  }
  return connectedFlag;
}

void WifiModule::disable() {
  enabledFlag = false;
  connectedFlag = false;
  WiFi.disconnect();
  WiFi.end();
  Serial.println(F("[WiFi] Disabled"));
}

bool WifiModule::ensureConnected(unsigned long now) {
  if (!enabledFlag) return false;
  auto status = WiFi.status();
  if (status == WL_CONNECTED && WiFi.localIP() != IPAddress(0,0,0,0)) {
    if (!connectedFlag) {
      IPAddress ip = WiFi.localIP();
      Serial.print(F("[WiFi] Reconnected, IP="));
      Serial.println(ip);
      server.begin(); // ensure server is listening after reconnect
    }
    connectedFlag = true;
    return true;
  }
  connectedFlag = false;
  const unsigned long RETRY_MS = 5000;
  if (now - lastConnectAttempt < RETRY_MS) return false;
  lastConnectAttempt = now;
  Serial.print(F("[WiFi] Reconnecting... status="));
  Serial.println(status);
  reconnectCount++;
  WiFi.disconnect();
  delay(50);
  WiFi.end();
  delay(50);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  return false;
}

int WifiModule::extractPresetId(const String &path) const {
  int idx = path.indexOf(F("id="));
  if (idx < 0) return -1;
  int start = idx + 3;
  int end = path.indexOf('&', start);
  String val = (end == -1) ? path.substring(start) : path.substring(start, end);
  int parsed = val.toInt();
  if (parsed <= 0 || parsed > 255) return -1;
  return parsed;
}

void WifiModule::sendResponse(WiFiClient &client, int code, const char *reason, const char *contentType, const String &body) {
  client.print(F("HTTP/1.1 "));
  client.print(code);
  client.print(' ');
  client.println(reason);
  client.print(F("Content-Type: "));
  client.println(contentType);
  client.print(F("Content-Length: "));
  client.println(body.length());
  client.println(F("Connection: close"));
  client.println();
  client.print(body);
}

void WifiModule::sendStatus(WiFiClient &client) {
  sendResponse(client, 200, "OK", "application/json", jsonStatus());
}

void WifiModule::sendDashboard(WiFiClient &client) {
  sendResponse(client, 200, "OK", "text/html", htmlPage());
}

String WifiModule::htmlPage() const {
  return String(DASHBOARD_HTML);
}

String WifiModule::jsonStatus() const {
  String body;
  body.reserve(512);
  body += F("{\"wifiEnabled\":");
  body += enabledFlag ? F("true") : F("false");
  body += F(",\"connected\":");
  body += connectedFlag ? F("true") : F("false");
  body += F(",\"ip\":\"");
  body += connectedFlag ? WiFi.localIP().toString() : F("");
  body += F("\",\"mdns\":\"\"");
  body += F(",\"running\":");
  body += (app && app->wifiIsRunning()) ? F("true") : F("false");
  body += F(",\"uptimeSec\":");
  body += String(millis() / 1000UL);

  const PresetConfig* active = app ? app->wifiActivePreset() : nullptr;
  const PresetConfig* sel = app ? app->wifiSelectedPreset() : nullptr;
  body += F(",\"mode\":\"");
  if (app && app->wifiManualIsActive()) body += F("manual");
  else if (active) body += F("preset");
  else body += F("idle");
  body += F("\",\"activePreset\":{");
  body += F("\"id\":");
  body += active ? String(active->id) : F("0");
  body += F(",\"label\":\"");
  if (active && active->label) body += active->label;
  body += F("\"}");
  body += F(",\"selectedPreset\":{");
  body += F("\"id\":");
  body += sel ? String(sel->id) : F("0");
  body += F(",\"label\":\"");
  if (sel && sel->label) body += sel->label;
  body += F("\"}");
  body += F(",\"lastPreset\":");
  body += app ? String(app->wifiLastRunPresetId()) : F("0");

  MotorSnapshot m1 = app ? app->wifiMotorSnapshot(MotorId::Motor1) : MotorSnapshot{};
  MotorSnapshot m2 = app ? app->wifiMotorSnapshot(MotorId::Motor2) : MotorSnapshot{};
  body += F(",\"metrics\":{");
  body += F("\"m1\":{\"cw\":");
  body += String(m1.turnsCW, 2);
  body += F(",\"ccw\":");
  body += String(m1.turnsCCW, 2);
  body += F(",\"tpm\":");
  body += String(m1.turnsPerMinute, 3);
  body += F(",\"tpd\":");
  body += String(m1.turnsPerDay, 2);
  body += F(",\"starts\":");
  body += String(m1.startCount);
  body += F(",\"runSec\":");
  body += String(m1.runSeconds);
  body += F("},\"m2\":{\"cw\":");
  body += String(m2.turnsCW, 2);
  body += F(",\"ccw\":");
  body += String(m2.turnsCCW, 2);
  body += F(",\"tpm\":");
  body += String(m2.turnsPerMinute, 3);
  body += F(",\"tpd\":");
  body += String(m2.turnsPerDay, 2);
  body += F(",\"starts\":");
  body += String(m2.startCount);
  body += F(",\"runSec\":");
  body += String(m2.runSeconds);
  body += F("}}");

  const bool sched = app ? app->wifiScheduleActive() : false;
  body += F(",\"schedule\":{");
  body += F("\"armed\":");
  body += sched ? F("true") : F("false");
  body += F(",\"presetId\":");
  body += sched ? String(app->wifiSchedulePresetId()) : F("0");
  body += F(",\"remaining\":");
  body += sched ? String(app->wifiScheduleSecondsRemaining(millis())) : F("0");
  body += F(",\"executed\":");
  body += app ? String(app->wifiScheduleRunCount()) : F("0");
  body += F("}");

  body += F(",\"manual\":{");
  body += F("\"active\":");
  bool manualActive = app ? app->wifiManualIsActive() : false;
  body += manualActive ? F("true") : F("false");
  body += F(",\"motors\":\"");
  if (manualActive) {
    MotorSelection msel = app->wifiManualMotors();
    body += (msel == MotorSelection::BOTH) ? F("both") : (msel == MotorSelection::MOTOR1 ? F("m1") : F("m2"));
  }
  body += F("\",\"dir1\":\"");
  if (manualActive) body += (app->wifiManualDir1() == StepperDir::CW ? F("cw") : F("ccw"));
  body += F("\",\"dir2\":\"");
  if (manualActive) body += (app->wifiManualDir2() == StepperDir::CW ? F("cw") : F("ccw"));
  body += F("\",\"speed\":\"");
  if (manualActive) {
    switch (app->wifiManualSpeed()) {
      case StepperSpeed::SLOW: body += F("slow"); break;
      case StepperSpeed::FAST: body += F("fast"); break;
      default: body += F("normal"); break;
    }
  }
  body += F("\"}");
  body += F(",\"wifiCounters\":{");
  body += F("\"requests\":");
  body += String(requestCount);
  body += F(",\"reconnects\":");
  body += String(reconnectCount);
  body += F("}");
  body += F("}");
  return body;
}

void WifiModule::sendHelp(WiFiClient &client) {
  String body;
  body.reserve(240);
  body += F("WatchWinder WiFi endpoints:\n");
  body += F("GET /status -> JSON state\n");
  body += F("GET /start?id=N -> select and start preset N\n");
  body += F("GET /stop -> stop active preset\n");
  body += F("GET /schedule?id=N&delay=secs -> start preset after delay\n");
  body += F("GET /manual?motors=both|m1|m2&dir1=cw|ccw&dir2=cw|ccw&speed=slow|normal|fast -> drive\n");
  sendResponse(client, 200, "OK", "text/plain", body);
}

void WifiModule::handleClient(WiFiClient &client) {
  client.setTimeout(10); // keep request parsing from stalling the loop
  requestCount++;
  String reqLine = client.readStringUntil('\r');
  client.read(); // consume '\n'

  // Consume the rest of the headers quickly.
  while (client.connected() && client.available()) {
    if (client.readStringUntil('\n').length() <= 1) break;
  }

  if (!reqLine.startsWith(F("GET "))) {
    sendResponse(client, 405, "Method Not Allowed", "text/plain", F("Only GET supported"));
    return;
  }

  int firstSpace = reqLine.indexOf(' ');
  int secondSpace = reqLine.indexOf(' ', firstSpace + 1);
  if (firstSpace < 0 || secondSpace < 0) {
    sendResponse(client, 400, "Bad Request", "text/plain", F("Malformed request"));
    return;
  }
  String path = reqLine.substring(firstSpace + 1, secondSpace);

  auto param = [](const String &p, const char *key) {
    String token = String(key) + '=';
    int idx = p.indexOf(token);
    if (idx < 0) return String();
    int start = idx + token.length();
    int end = p.indexOf('&', start);
    if (end < 0) end = p.length();
    return p.substring(start, end);
  };

  if (path == F("/") || path.startsWith(F("/index"))) {
    sendDashboard(client);
    return;
  }

  if (path.startsWith(F("/status"))) {
    sendStatus(client);
    return;
  }

  if (path.startsWith(F("/start"))) {
    if (!app) {
      sendResponse(client, 500, "Server Error", "text/plain", F("App not bound"));
      return;
    }
    int id = extractPresetId(path);
    if (id < 0) {
      sendResponse(client, 400, "Bad Request", "text/plain", F("Missing id"));
      return;
    }
    bool ok = app->wifiStartPreset(static_cast<uint8_t>(id), millis());
    if (ok) {
      sendResponse(client, 200, "OK", "text/plain", F("Started preset\n"));
    } else {
      sendResponse(client, 404, "Not Found", "text/plain", F("Preset not found or failed\n"));
    }
    return;
  }

  if (path.startsWith(F("/stop"))) {
    if (app) app->wifiStopPreset();
    sendResponse(client, 200, "OK", "text/plain", F("Stopped\n"));
    return;
  }

  if (path.startsWith(F("/schedule/cancel"))) {
    bool ok = app && app->wifiCancelSchedule();
    sendResponse(client, ok ? 200 : 400, ok ? "OK" : "Bad Request", "text/plain", ok ? F("Schedule cleared") : F("No schedule armed"));
    return;
  }

  if (path.startsWith(F("/schedule"))) {
    if (!app) {
      sendResponse(client, 500, "Server Error", "text/plain", F("App not bound"));
      return;
    }
    int id = extractPresetId(path);
    String delayStr = param(path, "delay");
    uint32_t delaySec = delayStr.length() ? (uint32_t)delayStr.toInt() : 0;
    if (id < 0 || delaySec == 0) {
      sendResponse(client, 400, "Bad Request", "text/plain", F("Need id and delay"));
      return;
    }
    bool ok = app->wifiSchedulePreset((uint8_t)id, delaySec);
    sendResponse(client, ok ? 200 : 404, ok ? "OK" : "Not Found", "text/plain", ok ? F("Scheduled\n") : F("Preset not found\n"));
    return;
  }

  if (path.startsWith(F("/manual/stop"))) {
    if (app) app->wifiManualStop();
    sendResponse(client, 200, "OK", "text/plain", F("Manual stopped"));
    return;
  }

  if (path.startsWith(F("/manual"))) {
    if (!app) {
      sendResponse(client, 500, "Server Error", "text/plain", F("App not bound"));
      return;
    }
    String motorsStr = param(path, "motors");
    String dir1Str = param(path, "dir1");
    String dir2Str = param(path, "dir2");
    String speedStr = param(path, "speed");
    MotorSelection motors = MotorSelection::BOTH;
    if (motorsStr == F("m1")) motors = MotorSelection::MOTOR1;
    else if (motorsStr == F("m2")) motors = MotorSelection::MOTOR2;
    StepperDir dir1 = (dir1Str == F("ccw")) ? StepperDir::CCW : StepperDir::CW;
    StepperDir dir2 = (dir2Str == F("ccw")) ? StepperDir::CCW : StepperDir::CW;
    StepperSpeed speed = StepperSpeed::NORMAL;
    if (speedStr == F("slow")) speed = StepperSpeed::SLOW;
    else if (speedStr == F("fast")) speed = StepperSpeed::FAST;
    app->wifiManualStart(motors, dir1, dir2, speed);
    sendResponse(client, 200, "OK", "text/plain", F("Manual running"));
    return;
  }

  sendHelp(client);
}

void WifiModule::tick(unsigned long now) {
  if (!enabledFlag) return;
  const bool running = app && (app->wifiIsRunning() || app->wifiManualIsActive());
  WiFiClient client = server.available();

  if (client) {
    client.setTimeout(10);
    if (!ensureConnected(now)) { client.stop(); return; }
    handleClient(client);
    client.stop();
    lastClientAt = now;
    lastTickMs = now;
    return;
  }

  // No pending client: if motors are running, skip WiFi entirely.
  if (running) return;

  if (connectedFlag && (now - lastTickMs) < IDLE_INTERVAL_MS) return;
  lastTickMs = now;
  ensureConnected(now);
}

#else  // WIFI_MODULE_ENABLED == 0

bool WifiModule::begin(WatchWinderApp *appPtr) {
  app = appPtr;
  enabledFlag = false;
  connectedFlag = false;
  return false;
}

void WifiModule::tick(unsigned long now) {
  (void)now;
}

#endif
