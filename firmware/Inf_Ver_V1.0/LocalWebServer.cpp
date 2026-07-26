#include "LocalWebServer.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFi.h>

#include "AppConfig.h"
#include "SensorManager.h"
#include "StateMachine.h"
#include "TokenManager.h"
#include "ValveManager.h"
#include "WiFiManager.h"

static const char DASHBOARD_PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>TARS Riego</title>
<style>
:root{
  --bg:#eef3f8;--surface:#fff;--border:#dfe7ef;--text:#172033;
  --muted:#748196;--blue:#2563eb;--blue2:#dbeafe;--green:#15945d;
  --green2:#dcfce7;--red:#dc3f3f;--red2:#fee2e2;--orange:#e97720;
  --purple:#7254db;--teal:#0d9488;--shadow:0 12px 32px rgba(15,23,42,.08);
}
*{box-sizing:border-box}body{margin:0;background:linear-gradient(145deg,#eaf0f7,#f8fafc);
font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;color:var(--text);min-height:100vh}
.wrap{max-width:1180px;margin:auto;padding:22px}
header{display:flex;align-items:center;gap:14px;background:var(--surface);padding:16px 20px;
border:1px solid var(--border);border-radius:18px;box-shadow:var(--shadow);margin-bottom:18px}
.logo{width:44px;height:44px;border-radius:13px;background:linear-gradient(135deg,var(--blue),var(--purple));
display:grid;place-items:center;color:#fff;font-weight:800}.titles h1{font-size:1.1rem;margin:0}
.titles p{font-size:.75rem;color:var(--muted);margin:3px 0 0}.header-right{margin-left:auto;text-align:right}
.state{font-size:.75rem;font-weight:700}.ip{font-size:.68rem;color:var(--muted);margin-top:3px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(270px,1fr));gap:16px}
.card{background:var(--surface);border:1px solid var(--border);border-radius:18px;padding:19px;
box-shadow:0 3px 12px rgba(15,23,42,.04);position:relative;overflow:hidden}
.card:before{content:"";position:absolute;top:0;left:0;right:0;height:3px;background:var(--accent,var(--blue))}
.card-head{display:flex;align-items:center;gap:10px;margin-bottom:15px}.icon{width:36px;height:36px;
border-radius:11px;background:var(--soft,var(--blue2));color:var(--accent,var(--blue));display:grid;place-items:center}
.name{font-size:.88rem;font-weight:750}.sub{font-size:.66rem;color:var(--muted);margin-top:2px}
.badge{margin-left:auto;border-radius:999px;padding:4px 9px;font-size:.62rem;font-weight:800;
background:#edf1f5;color:var(--muted)}.badge.ok{background:var(--green2);color:var(--green)}
.badge.err{background:var(--red2);color:var(--red)}.big{font-family:ui-monospace,SFMono-Regular,Consolas,monospace;
font-size:2.15rem;font-weight:800}.unit{font-size:.76rem;color:var(--muted);margin-left:5px}
.divider{height:1px;background:var(--border);margin:14px 0}.row{display:flex;justify-content:space-between;gap:12px}
.kv{display:flex;flex-direction:column;gap:3px}.k{font-size:.59rem;text-transform:uppercase;color:var(--muted)}
.v{font-family:ui-monospace,Consolas,monospace;font-size:.78rem;font-weight:700}
.valve-panel{text-align:center}.valve-state{font-size:1.8rem;font-weight:850;margin:5px 0 15px}
.valve-state.open{color:var(--green)}.valve-state.closed{color:var(--red)}
.actions{display:flex;gap:10px}.btn{border:0;border-radius:11px;padding:11px 14px;font-weight:800;
cursor:pointer;flex:1}.btn-open{background:var(--green);color:#fff}.btn-close{background:var(--red);color:#fff}
.btn-secondary{background:#e8eef6;color:#334155}.btn-link{display:inline-flex;text-decoration:none;
align-items:center;justify-content:center}.btn:disabled{opacity:.5;cursor:not-allowed}
.notice{font-size:.68rem;line-height:1.45;color:var(--muted);margin-top:11px}
#progressWrap{height:8px;background:#e8edf4;border-radius:999px;margin-top:12px;overflow:hidden;display:none}
#progress{height:100%;width:0;background:linear-gradient(90deg,var(--blue),var(--purple));transition:width .15s}
#otaStatus{font-size:.72rem;margin-top:10px;color:var(--muted)}
input[type=file]{width:100%;font-size:.75rem;padding:9px;border:1px dashed #c9d3df;border-radius:10px}
.server-line{font-size:.72rem;margin:7px 0;color:var(--muted);word-break:break-word}
.server-line strong{color:var(--text)}footer{text-align:center;color:#9aa6b5;font-size:.67rem;margin:20px}
@media(max-width:520px){.wrap{padding:12px}.actions{flex-direction:column}.header-right{display:none}}
</style>
</head>
<body>
<div class="wrap">
<header>
  <div class="logo">TR</div>
  <div class="titles">
    <h1 id="deviceTitle">TARS · Sistema de riego</h1>
    <p id="deviceAddress">SHT31 · DS18B20 · suelo RS485 · caudal · electroválvula</p>
  </div>
  <div class="header-right">
    <div class="state" id="machineState">INICIO</div>
    <div class="ip" id="networkInfo">Cargando red...</div>
  </div>
</header>

<div class="grid">
  <section class="card" style="--accent:#2563eb;--soft:#dbeafe">
    <div class="card-head"><div class="icon">💧</div><div><div class="name">Caudalímetro</div>
    <div class="sub">FS400A · GPIO 11</div></div><span class="badge" id="flowBadge">OFF</span></div>
    <div class="big"><span id="flowRate">0.000</span><span class="unit">L/min</span></div>
    <div class="divider"></div>
    <div class="row">
      <div class="kv"><span class="k">Frecuencia</span><span class="v" id="flowFreq">0 Hz</span></div>
      <div class="kv"><span class="k">Pulsos</span><span class="v" id="flowPulses">0</span></div>
      <div class="kv"><span class="k">Total</span><span class="v" id="flowTotal">0 L</span></div>
    </div>
    <button class="btn btn-secondary" style="width:100%;margin-top:14px" onclick="resetFlow()">Reiniciar total</button>
    <div class="notice">El conteo físico se habilita únicamente mientras la electroválvula está abierta.</div>
  </section>

  <section class="card valve-panel" style="--accent:#15945d;--soft:#dcfce7">
    <div class="card-head"><div class="icon">◉</div><div><div class="name">Electroválvula</div>
    <div class="sub">Control local · GPIO 12</div></div><span class="badge" id="valveBadge">CERRADA</span></div>
    <div class="valve-state closed" id="valveState">CERRADA</div>
    <div class="actions">
      <button class="btn btn-open" id="openBtn" onclick="setValve('open')">Abrir</button>
      <button class="btn btn-close" id="closeBtn" onclick="setValve('close')">Cerrar</button>
    </div>
    <div class="notice">La válvula inicia cerrada y también se cierra automáticamente antes de una actualización OTA.</div>
  </section>

  <section class="card" style="--accent:#e97720;--soft:#ffedd5">
    <div class="card-head"><div class="icon">🌡</div><div><div class="name">Temperatura ambiente</div>
    <div class="sub">DS18B20 · GPIO 10</div></div><span class="badge" id="dsBadge">...</span></div>
    <div class="big"><span id="dsTemp">–</span><span class="unit">°C</span></div>
  </section>

  <section class="card" style="--accent:#0d9488;--soft:#ccfbf1">
    <div class="card-head"><div class="icon">☁</div><div><div class="name">SHT31 · Bus 0</div>
    <div class="sub">SDA 4 · SCL 5 · 0x44</div></div><span class="badge" id="sht0Badge">...</span></div>
    <div class="big"><span id="sht0Temp">–</span><span class="unit">°C</span></div>
    <div class="divider"></div><div class="row"><div class="kv"><span class="k">Humedad</span>
    <span class="v" id="sht0Hum">– %</span></div></div>
  </section>

  <section class="card" style="--accent:#7254db;--soft:#ede9fe">
    <div class="card-head"><div class="icon">☁</div><div><div class="name">SHT31 · Bus 1</div>
    <div class="sub">SDA 8 · SCL 9 · 0x44</div></div><span class="badge" id="sht1Badge">...</span></div>
    <div class="big"><span id="sht1Temp">–</span><span class="unit">°C</span></div>
    <div class="divider"></div><div class="row"><div class="kv"><span class="k">Humedad</span>
    <span class="v" id="sht1Hum">– %</span></div></div>
  </section>

  <section class="card" style="--accent:#15945d;--soft:#dcfce7">
    <div class="card-head"><div class="icon">🌱</div><div><div class="name">Sensor de suelo</div>
    <div class="sub">JXBS-3001-TR · RS485</div></div><span class="badge" id="soilBadge">...</span></div>
    <div class="big"><span id="soilTemp">–</span><span class="unit">°C</span></div>
    <div class="divider"></div><div class="row"><div class="kv"><span class="k">Humedad</span>
    <span class="v" id="soilHum">– %</span></div></div>
  </section>

  <section class="card" style="--accent:#334155;--soft:#e2e8f0">
    <div class="card-head"><div class="icon">⇧</div><div><div class="name">Servidor FIWARE</div>
    <div class="sub">Estado del último PATCH</div></div><span class="badge" id="sendBadge">PENDIENTE</span></div>
    <div class="server-line"><strong>HTTP:</strong> <span id="httpCode">0</span></div>
    <div class="server-line"><strong>Mensaje:</strong> <span id="sendMessage">Sin envíos</span></div>
    <div class="server-line"><strong>Próximo envío:</strong> <span id="nextSend">–</span></div>
    <a href="/config" class="btn btn-secondary btn-link" style="width:100%;margin-top:10px">Configuración</a>
  </section>

  <section class="card" style="--accent:#7254db;--soft:#ede9fe">
    <div class="card-head"><div class="icon">↻</div><div><div class="name">Web OTA</div>
    <div class="sub">Actualizar firmware desde archivo .bin</div></div></div>
    <input type="file" id="firmware" accept=".bin,application/octet-stream">
    <button class="btn btn-secondary" style="width:100%;margin-top:10px" onclick="uploadFirmware()">Subir firmware</button>
    <div id="progressWrap"><div id="progress"></div></div>
    <div id="otaStatus">La válvula se cerrará antes de comenzar.</div>
  </section>
</div>
<footer>TARS Riego · ESP32-S3 · dashboard local</footer>
</div>

<script>
const $=id=>document.getElementById(id);
const val=(v,d=2)=>Number.isFinite(Number(v))?Number(v).toFixed(d):'–';
function badge(id,ok,good='OK',bad='ERR'){
  const e=$(id);e.textContent=ok?good:bad;e.className='badge '+(ok?'ok':'err');
}
async function refresh(){
  try{
    const r=await fetch('/api/data',{cache:'no-store'});
    if(!r.ok)throw new Error('HTTP '+r.status);
    const d=await r.json();

    $('deviceTitle').textContent=d.deviceName;
    document.title=d.deviceName;
    $('deviceAddress').textContent=d.localUrl+' · AP: '+d.apSSID;
    $('machineState').textContent=d.state;
    $('networkInfo').textContent=(d.wifiConnected?'WiFi ':'AP ')+d.ip;

    const open=d.valveOpen;
    $('valveState').textContent=open?'ABIERTA':'CERRADA';
    $('valveState').className='valve-state '+(open?'open':'closed');
    $('valveBadge').textContent=open?'ABIERTA':'CERRADA';
    $('valveBadge').className='badge '+(open?'ok':'err');
    $('openBtn').disabled=open;$('closeBtn').disabled=!open;

    $('flowRate').textContent=val(d.flowRate,3);
    $('flowFreq').textContent=val(d.flowFreq,2)+' Hz';
    $('flowPulses').textContent=d.flowPulses;
    $('flowTotal').textContent=val(d.flowTotal,3)+' L';
    $('flowBadge').textContent=d.flowEnabled?(d.flowPulses>0?'FLOW':'ACTIVO'):'BLOQUEADO';
    $('flowBadge').className='badge '+(d.flowEnabled?'ok':'err');

    $('dsTemp').textContent=d.dsOk?val(d.ambientTemp,2):'–';badge('dsBadge',d.dsOk);
    $('sht0Temp').textContent=d.sht0Ok?val(d.sht0Temp,2):'–';
    $('sht0Hum').textContent=d.sht0Ok?val(d.sht0Hum,1)+' %':'– %';badge('sht0Badge',d.sht0Ok);
    $('sht1Temp').textContent=d.sht1Ok?val(d.sht1Temp,2):'–';
    $('sht1Hum').textContent=d.sht1Ok?val(d.sht1Hum,1)+' %':'– %';badge('sht1Badge',d.sht1Ok);
    $('soilTemp').textContent=d.soilOk?val(d.soilTemp,1):'–';
    $('soilHum').textContent=d.soilOk?val(d.soilHum,1)+' %':'– %';badge('soilBadge',d.soilOk);

    $('httpCode').textContent=d.lastHttpCode;
    $('sendMessage').textContent=d.lastSendMessage;
    $('nextSend').textContent=Math.max(0,Math.ceil(d.nextSendMs/1000))+' s';
    $('sendBadge').textContent=d.lastSendAt===0?'PENDIENTE':(d.lastSendOk?'ENVIADO':'ERROR');
    $('sendBadge').className='badge '+(d.lastSendAt===0?'':(d.lastSendOk?'ok':'err'));
  }catch(e){
    $('networkInfo').textContent='Sin respuesta local';
  }
}
async function setValve(action){
  if(action==='open'&&!confirm('¿Abrir la electroválvula?'))return;
  const r=await fetch('/api/valve',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'action='+encodeURIComponent(action)});
  const d=await r.json();
  if(!d.ok)alert(d.message);
  refresh();
}
async function resetFlow(){
  if(!confirm('¿Reiniciar el total acumulado del caudalímetro?'))return;
  const r=await fetch('/api/reset-flow',{method:'POST'});
  const d=await r.json();if(!d.ok)alert(d.message);refresh();
}
function uploadFirmware(){
  const input=$('firmware'),file=input.files[0],status=$('otaStatus');
  if(!file){status.textContent='Selecciona un archivo .bin';return}
  if(!file.name.toLowerCase().endsWith('.bin')){status.textContent='El archivo debe terminar en .bin';return}
  if(!confirm('La válvula se cerrará y el ESP32 se reiniciará. ¿Continuar?'))return;

  const form=new FormData();form.append('firmware',file);
  const xhr=new XMLHttpRequest();
  $('progressWrap').style.display='block';$('progress').style.width='0%';
  status.textContent='Cargando firmware...';
  xhr.upload.onprogress=e=>{
    if(e.lengthComputable)$('progress').style.width=Math.round(e.loaded/e.total*100)+'%';
  };
  xhr.onload=()=>{
    if(xhr.status===200){
      $('progress').style.width='100%';
      status.textContent='Actualización correcta. Reiniciando...';
    }else status.textContent='Error OTA: '+xhr.responseText;
  };
  xhr.onerror=()=>status.textContent='Error de conexión durante OTA';
  xhr.open('POST','/ota');xhr.send(form);
}
refresh();setInterval(refresh,1000);
</script>
</body>
</html>
)HTML";

String LocalWebServer::htmlEscape(const String& input) {
  String output;
  output.reserve(input.length() + 16);

  for (size_t i = 0; i < input.length(); i++) {
    switch (input[i]) {
      case '&': output += "&amp;"; break;
      case '<': output += "&lt;"; break;
      case '>': output += "&gt;"; break;
      case '"': output += "&quot;"; break;
      case '\'': output += "&#39;"; break;
      default: output += input[i];
    }
  }
  return output;
}

void LocalWebServer::begin() {
  if (initialized) return;

  registerRoutes();
  server->begin();
  initialized = true;

  Serial.println("[Web] Servidor local iniciado en puerto 80");
  Serial.printf("[Web] Dirección actual: http://%s\n",
                wifiManager.getIP().c_str());
}

void LocalWebServer::handle() {
  if (!initialized) return;

  server->handleClient();

  if (!mdnsStarted && wifiManager.isConnected()) {
    mdnsStarted = MDNS.begin(appConfig.hostname.c_str());
    if (mdnsStarted) {
      MDNS.addService("http", "tcp", 80);
      Serial.printf("[mDNS] http://%s.local\n", appConfig.hostname.c_str());
    }
  }

  if (restartScheduled &&
      static_cast<long>(millis() - restartAt) >= 0) {
    valveManager.close();
    sensorManager.persistFlowTotal();
    delay(100);
    ESP.restart();
  }
}

void LocalWebServer::registerRoutes() {
  server->on("/", HTTP_GET, [this]() { handleRoot(); });
  server->on("/api/data", HTTP_GET, [this]() { handleData(); });
  server->on("/data", HTTP_GET, [this]() { handleData(); });

  server->on("/api/valve", HTTP_POST, [this]() { handleValve(); });
  server->on("/api/reset-flow", HTTP_POST,
             [this]() { handleFlowReset(); });

  server->on("/config", HTTP_GET, [this]() { handleConfigPage(); });
  server->on("/config", HTTP_POST, [this]() { handleConfigSave(); });

  server->on("/health", HTTP_GET, [this]() {
    server->send(200, "application/json",
                 "{\"ok\":true,\"service\":\"tars-riego\"}");
  });

  server->on(
      "/ota", HTTP_POST,
      [this]() {
        server->sendHeader("Connection", "close");

        if (otaError || !otaStarted || Update.hasError()) {
          server->send(500, "text/plain",
                       "La actualización OTA falló");
        } else {
          server->send(200, "text/plain",
                       "OK. El ESP32 se reiniciará.");
          restartScheduled = true;
          restartAt = millis() + 1500;
        }
      },
      [this]() {
        HTTPUpload& upload = server->upload();

        if (upload.status == UPLOAD_FILE_START) {
          otaError = false;
          otaStarted = false;

          String filename = upload.filename;
          filename.toLowerCase();

          if (!filename.endsWith(".bin")) {
            otaError = true;
            Serial.println("[OTA] Archivo rechazado: no es .bin");
            return;
          }

          // Estado seguro antes de escribir flash.
          valveManager.close();
          sensorManager.persistFlowTotal();

          Serial.printf("[OTA] Inicio: %s\n", upload.filename.c_str());

          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            otaError = true;
            Update.printError(Serial);
            return;
          }

          otaStarted = true;
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (otaError || !otaStarted) return;

          size_t written = Update.write(upload.buf, upload.currentSize);
          if (written != upload.currentSize) {
            otaError = true;
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (otaError || !otaStarted) return;

          if (!Update.end(true)) {
            otaError = true;
            Update.printError(Serial);
          } else {
            Serial.printf("[OTA] Completado: %u bytes\n",
                          upload.totalSize);
          }
        } else if (upload.status == UPLOAD_FILE_ABORTED) {
          otaError = true;
          Update.end(false);
          Serial.println("[OTA] Carga abortada");
        }
      });

  server->onNotFound([this]() {
    server->send(404, "application/json",
                 "{\"ok\":false,\"message\":\"Ruta no encontrada\"}");
  });
}

void LocalWebServer::handleRoot() {
  server->sendHeader("Cache-Control", "no-store");
  server->send_P(200, "text/html; charset=utf-8", DASHBOARD_PAGE);
}

void LocalWebServer::handleData() {
  SensorData data = sensorManager.snapshot();

  JsonDocument doc;
  doc["state"] = stateMachine.getCurrentStateName();
  doc["uptimeMs"] = millis();

  doc["deviceName"] = appConfig.hostname;
  doc["localUrl"] = appConfig.getLocalUrl();
  doc["apSSID"] = appConfig.getSetupSSID();
  doc["wifiConnected"] = wifiManager.isConnected();
  doc["ip"] = wifiManager.getIP();

  doc["valveOpen"] = valveManager.isOpen();
  doc["flowEnabled"] = data.flowEnabled;
  doc["flowRate"] = data.flowRateLMin;
  doc["flowTotal"] = data.totalLiters;
  doc["flowFreq"] = data.flowFreqHz;
  doc["flowPulses"] = data.pulsesLastWindow;

  doc["ambientTemp"] = data.ambientTempC;
  doc["dsOk"] = data.dsOk;

  doc["sht0Temp"] = data.sht0TempC;
  doc["sht0Hum"] = data.sht0HumPct;
  doc["sht0Ok"] = data.sht0Ok;

  doc["sht1Temp"] = data.sht1TempC;
  doc["sht1Hum"] = data.sht1HumPct;
  doc["sht1Ok"] = data.sht1Ok;

  doc["soilTemp"] = data.soilTempC;
  doc["soilHum"] = data.soilHumPct;
  doc["soilOk"] = data.soilOk;

  doc["lastSendOk"] = stateMachine.sendStatus.lastSendOk;
  doc["lastHttpCode"] = stateMachine.sendStatus.lastHttpCode;
  doc["lastSendAt"] = stateMachine.sendStatus.lastSendAt;
  doc["lastSendMessage"] = stateMachine.sendStatus.lastMessage;

  long remaining = static_cast<long>(
      stateMachine.clocks.nextSend - millis());
  doc["nextSendMs"] = remaining > 0 ? remaining : 0;

  String json;
  serializeJson(doc, json);

  server->sendHeader("Cache-Control", "no-store");
  server->send(200, "application/json", json);
}

void LocalWebServer::handleValve() {
  if (!server->hasArg("action")) {
    sendJsonMessage(400, false, "Falta action=open o action=close");
    return;
  }

  String action = server->arg("action");
  action.toLowerCase();

  if (action == "open") {
    valveManager.open();
    sendJsonMessage(200, true, "Electroválvula abierta");
  } else if (action == "close") {
    valveManager.close();
    sendJsonMessage(200, true, "Electroválvula cerrada");
  } else {
    sendJsonMessage(400, false, "Acción desconocida");
  }
}

void LocalWebServer::handleFlowReset() {
  sensorManager.resetFlowTotal();
  sendJsonMessage(200, true, "Total de caudal reiniciado");
}

void LocalWebServer::handleConfigPage() {
  String page;
  page.reserve(8000);

  page = R"HTML(<!DOCTYPE html><html lang="es"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Configuración TARS Riego</title><style>
body{font-family:system-ui;background:#eef3f8;color:#172033;margin:0;padding:20px}
.wrap{max-width:760px;margin:auto}.card{background:#fff;border:1px solid #dfe7ef;border-radius:16px;
padding:20px;margin-bottom:15px;box-shadow:0 8px 24px rgba(15,23,42,.06)}
h1{font-size:1.35rem}h2{font-size:1rem;margin-top:0}label{font-size:.75rem;font-weight:700;
display:block;margin-top:12px}input,select{width:100%;padding:10px;margin-top:5px;border:1px solid #cbd5e1;
border-radius:9px;box-sizing:border-box}button,a{display:inline-block;border:0;border-radius:10px;padding:11px 15px;
font-weight:750;text-decoration:none;cursor:pointer}.save{background:#2563eb;color:#fff;width:100%;margin-top:16px}
.back{background:#e2e8f0;color:#334155}.note{font-size:.72rem;color:#64748b;line-height:1.45}
</style></head><body><div class="wrap"><h1>Configuración TARS Riego</h1>
<a class="back" href="/">← Volver al dashboard</a>
<form method="POST" action="/config">
<div class="card"><h2>Identidad del dispositivo</h2>
<label>Nombre único</label><input name="deviceName" minlength="3" maxlength="24" value=")HTML";
  page += htmlEscape(appConfig.hostname);
  page += R"HTML(">
<p class="note">Asigne un nombre diferente a cada ESP, por ejemplo <b>tars-riego-invernadero-01</b>. Se usará como <b>http://nombre.local</b> y también para el AP <b>NOMBRE-SETUP</b>. Solo se permiten letras, números y guiones.</p>
<p class="note">Dirección actual: <b>)HTML";
  page += htmlEscape(appConfig.getLocalUrl());
  page += R"HTML(</b><br>AP actual: <b>)HTML";
  page += htmlEscape(appConfig.getSetupSSID());
  page += R"HTML(</b></p></div>
<div class="card"><h2>Red WiFi</h2>
<label>SSID nuevo</label><input name="ssid" placeholder="Déjalo vacío para conservar el actual">
<label>Contraseña nueva</label><input type="password" name="wifiPass" placeholder="Solo se usa si escribes un SSID">
<p class="note">Al cambiar el WiFi, el dispositivo se reinicia. Si no conecta, publica el AP único indicado arriba.</p></div>
<div class="card"><h2>FIWARE / Orion</h2>
<label>URL completa del atributo de la entidad</label><input name="serverUrl" value=")HTML";

  page += htmlEscape(appConfig.serverUrl);
  page += R"HTML("><label>Intervalo de envío (ms)</label><input type="number" min="5000" name="sendMs" value=")HTML";
  page += String(appConfig.intervaloEnvio);
  page += R"HTML("><label>Intervalo de reintento (ms)</label><input type="number" min="5000" name="retryMs" value=")HTML";
  page += String(appConfig.intervaloReintento);
  page += R"HTML("><p class="note">El nombre local es independiente de la entidad FIWARE. Configure una URL de entidad distinta para cada equipo.</p></div><div class="card"><h2>Keyrock</h2>
<label>URL del token</label><input name="tokenUrl" value=")HTML";
  page += htmlEscape(appConfig.tokenUrl);
  page += R"HTML("><label>Client ID</label><input name="clientId" value=")HTML";
  page += htmlEscape(appConfig.clientId);
  page += R"HTML("><label>Client Secret</label><input type="password" name="clientSecret" placeholder="Vacío = conservar">
<label>Usuario</label><input name="keyrockUser" value=")HTML";
  page += htmlEscape(appConfig.keyrockUser);
  page += R"HTML("><label>Contraseña</label><input type="password" name="keyrockPass" placeholder="Vacío = conservar">
<label>Saltar token</label><select name="skipToken"><option value="0")HTML";
  if (!appConfig.skipToken) page += " selected";
  page += R"HTML(>Desactivado</option><option value="1")HTML";
  if (appConfig.skipToken) page += " selected";
  page += R"HTML(>Activado</option></select></div>
<div class="card"><h2>Sensores</h2><label>Intervalo de sensores lentos (ms)</label>
<input type="number" min="1000" name="sensorMs" value=")HTML";
  page += String(appConfig.intervaloSensores);
  page += R"HTML("><label>Calibración del caudalímetro (pulsos/litro)</label>
<input type="number" min="1" step="0.001" name="ppl" value=")HTML";
  page += String(appConfig.pulsosPorLitro, 3);
  page += R"HTML("><p class="note">El valor original 4.8 Hz por L/min equivale a 288 pulsos por litro.</p>
</div><button class="save" type="submit">Guardar configuración</button></form></div></body></html>)HTML";

  server->send(200, "text/html; charset=utf-8", page);
}

void LocalWebServer::handleConfigSave() {
  bool needsRestart = false;
  bool deviceNameChanged = false;

  if (server->hasArg("serverUrl") &&
      server->arg("serverUrl").length() > 0) {
    appConfig.serverUrl = server->arg("serverUrl");
  }

  if (server->hasArg("deviceName") &&
      server->arg("deviceName").length() > 0 &&
      server->arg("deviceName") != appConfig.hostname) {
    if (!appConfig.setDeviceName(server->arg("deviceName"))) {
      server->send(400, "text/plain; charset=utf-8",
                   "Nombre inválido. Use al menos 3 letras, números o guiones.");
      return;
    }
    needsRestart = true;
    deviceNameChanged = true;
  }

  if (server->hasArg("sendMs")) {
    unsigned long value = server->arg("sendMs").toInt();
    if (value >= 5000) appConfig.intervaloEnvio = value;
  }

  if (server->hasArg("retryMs")) {
    unsigned long value = server->arg("retryMs").toInt();
    if (value >= 5000) appConfig.intervaloReintento = value;
  }

  if (server->hasArg("sensorMs")) {
    unsigned long value = server->arg("sensorMs").toInt();
    if (value >= 1000) appConfig.intervaloSensores = value;
  }

  if (server->hasArg("ppl")) {
    float value = server->arg("ppl").toFloat();
    if (value >= 1.0f) appConfig.pulsosPorLitro = value;
  }

  if (server->hasArg("tokenUrl") &&
      server->arg("tokenUrl").length() > 0) {
    appConfig.tokenUrl = server->arg("tokenUrl");
  }
  if (server->hasArg("clientId") &&
      server->arg("clientId").length() > 0) {
    appConfig.clientId = server->arg("clientId");
  }
  if (server->hasArg("clientSecret") &&
      server->arg("clientSecret").length() > 0) {
    appConfig.clientSecret = server->arg("clientSecret");
  }
  if (server->hasArg("keyrockUser") &&
      server->arg("keyrockUser").length() > 0) {
    appConfig.keyrockUser = server->arg("keyrockUser");
  }
  if (server->hasArg("keyrockPass") &&
      server->arg("keyrockPass").length() > 0) {
    appConfig.keyrockPass = server->arg("keyrockPass");
  }

  appConfig.skipToken =
      server->hasArg("skipToken") &&
      server->arg("skipToken") == "1";

  if (server->hasArg("ssid") &&
      server->arg("ssid").length() > 0) {
    wifiManager.saveCredentials(server->arg("ssid"),
                                server->arg("wifiPass"));
    needsRestart = true;
  }

  appConfig.save();
  tokenManager.clear();

  const String redirectTarget =
      deviceNameChanged ? appConfig.getLocalUrl() : String("/");

  String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  page += "<meta http-equiv='refresh' content='4;url=" +
          redirectTarget + "'><title>Guardado</title></head>";
  page += "<body style='font-family:system-ui;text-align:center;padding:50px;background:#eef3f8'>";
  page += "<div style='max-width:520px;margin:auto;background:white;padding:30px;border-radius:16px'>";
  page += "<h1 style='color:#15945d'>Configuración guardada</h1><p>";

  if (deviceNameChanged) {
    page += "El ESP32 se reiniciará. Nueva dirección: <b>" +
            appConfig.getLocalUrl() + "</b><br>Nuevo AP de respaldo: <b>" +
            appConfig.getSetupSSID() + "</b>";
  } else if (needsRestart) {
    page += "El ESP32 se reiniciará para aplicar la nueva red WiFi.";
  } else {
    page += "Volviendo al dashboard...";
  }
  page += "</p></div></body></html>";

  server->send(200, "text/html; charset=utf-8", page);

  if (needsRestart) {
    valveManager.close();
    restartScheduled = true;
    restartAt = millis() + 1800;
  }
}

void LocalWebServer::sendJsonMessage(int code, bool ok,
                                     const String& message) {
  JsonDocument doc;
  doc["ok"] = ok;
  doc["message"] = message;
  doc["valveOpen"] = valveManager.isOpen();

  String json;
  serializeJson(doc, json);
  server->send(code, "application/json", json);
}
