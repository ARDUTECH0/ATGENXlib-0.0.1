#include "ATG_WiFiPortal.h"

#if defined(ESP32) || defined(ESP8266)

// ── Platform headers ──────────────────────────────────────────────
#if defined(ESP32)
  #include <WiFi.h>
  #include <DNSServer.h>
  #include <Preferences.h>
  static Preferences prefs;
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <DNSServer.h>
  #include <LittleFS.h>
  // credentials stored as /wifi_ssid.txt and /wifi_pass.txt
#endif

namespace atg {

// ── Static instances ──────────────────────────────────────────────
static PlatformWebServer server(80);
static DNSServer          dns;

// ── encryptionType helper ─────────────────────────────────────────
// ESP32 returns wifi_auth_mode_t; ESP8266 returns wl_enc_type.
// Both: 0 / OPEN means no password.
static bool _isEncrypted(int idx) {
#if defined(ESP32)
  return WiFi.encryptionType(idx) != WIFI_AUTH_OPEN;
#elif defined(ESP8266)
  return WiFi.encryptionType(idx) != ENC_TYPE_NONE;
#endif
}

// ═══════════════════════════════════════════════════════════════════
//  Constructor
// ═══════════════════════════════════════════════════════════════════
WiFiPortal::WiFiPortal(const char* productName, const char* apPass,
                       uint32_t portalTimeoutMs, uint32_t connectTimeoutMs)
  : _productName(productName),
    _apPass(apPass),
    _portalTimeoutMs(portalTimeoutMs),
    _connectTimeoutMs(connectTimeoutMs)
{}

// ═══════════════════════════════════════════════════════════════════
//  begin
// ═══════════════════════════════════════════════════════════════════
Result WiFiPortal::begin(Runtime& rt) {
  (void)rt;

  if (_apPass && strlen(_apPass) > 0 && strlen(_apPass) < 8) {
    Log::w(name(), F("AP password < 8 chars — AP will be OPEN"));
  }

#if defined(ESP8266)
  LittleFS.begin();
#endif

  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_STA);

  String ssid, pass;
  if (loadCredentials(ssid, pass)) {
    Log::i(name(), String(F("Saved SSID: ")) + ssid + F(" — connecting…"));
    tryConnect(ssid, pass);
    return Result::Ok;
  }

  startPortal();
  return Result::Ok;
}

// ═══════════════════════════════════════════════════════════════════
//  tick
// ═══════════════════════════════════════════════════════════════════
void WiFiPortal::tick(Runtime& rt) {
  (void)rt;

  // ── Connected ─────────────────────────────────────────
  if (WiFi.status() == WL_CONNECTED) {
    if (!_connected) {
      _connected  = true;
      _connecting = false;
      _ip         = WiFi.localIP().toString();
      Log::i(name(), String(F("✔ Connected  IP: ")) + _ip);
      if (_onConnected) _onConnected();
    }
    return;
  }

  // ── Lost connection ───────────────────────────────────
  if (_connected) {
    _connected = false;
    Log::w(name(), F("Connection lost"));
    if (_onDisconnected) _onDisconnected();
  }

  // ── Waiting for STA connect result ────────────────────
  if (_connecting && !_portalRunning) {
    if (_connectTimeoutMs > 0 && elapsed(_connectStart, _connectTimeoutMs)) {
      Log::w(name(), F("Connect timeout — starting portal"));
      _connecting = false;
      startPortal();
    }
    return;
  }

  // ── Portal loop ───────────────────────────────────────
  if (_portalRunning) {
    dns.processNextRequest();
    server.handleClient();

    if (_portalTimeoutMs > 0 && elapsed(_portalStart, _portalTimeoutMs)) {
      Log::i(name(), F("Portal timeout — stopping"));
      stopPortal();
    }
  }
}

// ═══════════════════════════════════════════════════════════════════
//  startPortal
// ═══════════════════════════════════════════════════════════════════
void WiFiPortal::startPortal() {
  if (_portalRunning) return;

  // Build unique AP name from chip ID
  char apName[64];
#if defined(ESP32)
  uint32_t chip = (uint32_t)(ESP.getEfuseMac() >> 16);
#elif defined(ESP8266)
  uint32_t chip = ESP.getChipId();
#endif
  snprintf(apName, sizeof(apName), "%s-Setup-%06X", _productName, chip & 0xFFFFFF);

  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP_STA);
  WiFi.scanNetworks(true /*async*/);

  const char* pwd = (_apPass && strlen(_apPass) >= 8) ? _apPass : nullptr;
  WiFi.softAP(apName, pwd);

  const IPAddress ip = WiFi.softAPIP();
  Log::i(name(), String(F("AP: ")) + apName + F("  IP: ") + ip.toString());

  dns.start(53, "*", ip);
  server.stop();
  setupServer();

  _portalRunning = true;
  _portalStart   = nowMs();
  if (_onPortalStart) _onPortalStart();
}

// ═══════════════════════════════════════════════════════════════════
//  stopPortal
// ═══════════════════════════════════════════════════════════════════
void WiFiPortal::stopPortal() {
  if (!_portalRunning) return;
  dns.stop();
  server.stop();
  WiFi.softAPdisconnect(true);
  _portalRunning = false;
  Log::i(name(), F("Portal stopped"));
}

// ═══════════════════════════════════════════════════════════════════
//  Credentials  —  ESP32: Preferences  |  ESP8266: LittleFS
// ═══════════════════════════════════════════════════════════════════
bool WiFiPortal::loadCredentials(String& ssid, String& pass) {
#if defined(ESP32)
  prefs.begin("atg_wifi", true);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
#elif defined(ESP8266)
  File f = LittleFS.open("/atg_ssid.txt", "r");
  if (!f) return false;
  ssid = f.readStringUntil('\n'); f.close();
  ssid.trim();
  File f2 = LittleFS.open("/atg_pass.txt", "r");
  if (f2) { pass = f2.readStringUntil('\n'); f2.close(); pass.trim(); }
#endif
  return ssid.length() > 0;
}

void WiFiPortal::saveCredentials(const String& ssid, const String& pass) {
#if defined(ESP32)
  prefs.begin("atg_wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
#elif defined(ESP8266)
  File f = LittleFS.open("/atg_ssid.txt", "w");
  if (f) { f.println(ssid); f.close(); }
  File f2 = LittleFS.open("/atg_pass.txt", "w");
  if (f2) { f2.println(pass); f2.close(); }
#endif
}

void WiFiPortal::resetCredentials() {
#if defined(ESP32)
  prefs.begin("atg_wifi", false);
  prefs.remove("ssid");
  prefs.remove("pass");
  prefs.end();
#elif defined(ESP8266)
  LittleFS.remove("/atg_ssid.txt");
  LittleFS.remove("/atg_pass.txt");
#endif
  Log::i(name(), F("Credentials cleared"));
}

// ═══════════════════════════════════════════════════════════════════
//  tryConnect
// ═══════════════════════════════════════════════════════════════════
void WiFiPortal::tryConnect(const String& ssid, const String& pass) {
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  _connecting   = true;
  _connectStart = nowMs();
}

// ═══════════════════════════════════════════════════════════════════
//  buildScanJson
// ═══════════════════════════════════════════════════════════════════
String WiFiPortal::buildScanJson() {
  int n = WiFi.scanComplete();

  if (n == WIFI_SCAN_RUNNING) {
    return F("{\"scanning\":true,\"networks\":[]}");
  }
  if (n <= 0) {
    WiFi.scanNetworks(true);
    return F("{\"scanning\":true,\"networks\":[]}");
  }

  String json;
  json.reserve(64 + n * 80);
  json = F("{\"scanning\":false,\"networks\":[");

  for (int i = 0; i < n; i++) {
    if (i) json += ',';
    String ess = WiFi.SSID(i);
    ess.replace("\\", "\\\\");
    ess.replace("\"", "\\\"");
    json += '{';
    json += "\"ssid\":\"" + ess + "\",";
    json += "\"rssi\":"   + String(WiFi.RSSI(i)) + ',';
    json += "\"enc\":"    + String(_isEncrypted(i) ? 1 : 0);
    json += '}';
  }
  json += "]}";

  WiFi.scanDelete();
  WiFi.scanNetworks(true);
  return json;
}

// ═══════════════════════════════════════════════════════════════════
//  setupServer
// ═══════════════════════════════════════════════════════════════════
void WiFiPortal::setupServer() {
  server.on("/", HTTP_GET, [&]() {
    server.send(200, "text/html; charset=utf-8", uiIndexHtml());
  });

  server.on("/scan", HTTP_GET, [&]() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", buildScanJson());
  });

  server.on("/scan", HTTP_POST, [&]() {
    WiFi.scanNetworks(true);
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/save", HTTP_POST, [&]() {
    const String ssid = server.arg("ssid");
    const String pass = server.arg("pass");

    if (ssid.length() < 1) {
      server.send(400, "application/json", "{\"error\":\"SSID required\"}");
      return;
    }

    saveCredentials(ssid, pass);
    server.send(200, "application/json", "{\"ok\":true}");
    if (_onSaved) _onSaved();

    delay(400);
    stopPortal();
    tryConnect(ssid, pass);
  });

  // Captive portal catch-all
  server.onNotFound([&]() {
    const String host = server.hostHeader();
    const String apIp = WiFi.softAPIP().toString();
    if (host.length() && host != apIp) {
      server.sendHeader("Location", "http://" + apIp + "/", true);
      server.send(302, "text/plain", "");
    } else {
      server.send(200, "text/html; charset=utf-8", uiIndexHtml());
    }
  });

  server.begin();
}

// ═══════════════════════════════════════════════════════════════════
//  uiIndexHtml  — single-page WiFi setup UI
// ═══════════════════════════════════════════════════════════════════
String WiFiPortal::uiIndexHtml() const {
  String h;
  h.reserve(7000);

  h += R"rawhtml(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Wi-Fi Setup</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#09090f;--surface:#0f1117;--surface2:#161820;
  --border:#ffffff0d;--border2:#ffffff18;
  --accent:#4f8eff;--accent2:#7b5ea7;
  --success:#22c55e;--error:#ef4444;
  --text:#f1f5f9;--muted:#64748b;
}
body{background:var(--bg);color:var(--text);font-family:'Segoe UI',system-ui,sans-serif;
  min-height:100vh;display:flex;align-items:flex-start;justify-content:center;padding:24px 16px 48px;}
body::before{content:'';position:fixed;inset:0;
  background:radial-gradient(ellipse 80% 50% at 20% -10%,#4f8eff18 0%,transparent 60%),
             radial-gradient(ellipse 60% 40% at 90% 110%,#7b5ea720 0%,transparent 60%);
  pointer-events:none;z-index:0;}
.wrap{position:relative;z-index:1;width:100%;max-width:440px}
.header{text-align:center;margin-bottom:28px;padding-top:8px}
.logo-ring{width:56px;height:56px;border-radius:16px;
  background:linear-gradient(135deg,var(--accent),var(--accent2));
  display:flex;align-items:center;justify-content:center;
  margin:0 auto 14px;box-shadow:0 0 32px #4f8eff44;font-size:22px;}
.header h1{font-size:22px;font-weight:700;letter-spacing:-.3px}
.header p{margin-top:4px;color:var(--muted);font-size:13.5px}
.card{background:var(--surface);border:1px solid var(--border);border-radius:20px;
  padding:22px;box-shadow:0 2px 24px #0006,0 1px 2px #0003;}
.section-title{font-size:11px;font-weight:600;letter-spacing:.08em;text-transform:uppercase;
  color:var(--muted);margin-bottom:10px;display:flex;align-items:center;gap:8px;}
.section-title::after{content:'';flex:1;height:1px;background:var(--border2)}
#net-list{display:flex;flex-direction:column;gap:6px;margin-bottom:18px;min-height:56px;}
.net-item{display:flex;align-items:center;gap:12px;padding:12px 14px;border-radius:13px;
  background:var(--surface2);border:1px solid var(--border);
  cursor:pointer;transition:border-color .18s,background .18s;user-select:none;}
.net-item:hover{border-color:var(--accent);background:#4f8eff0a}
.net-item.selected{border-color:var(--accent);background:#4f8eff12}
.net-info{flex:1;min-width:0}
.net-ssid{font-size:14px;font-weight:600;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.net-sub{font-size:11.5px;color:var(--muted);margin-top:1px}
.net-lock{color:var(--muted);flex-shrink:0;font-size:13px}
.bars{display:flex;align-items:flex-end;gap:2px;height:16px;flex-shrink:0}
.bar{width:4px;border-radius:2px;background:var(--border2)}
.bar.on{background:var(--accent)}
.skeleton{height:52px;border-radius:13px;
  background:linear-gradient(90deg,var(--surface2) 25%,#ffffff07 50%,var(--surface2) 75%);
  background-size:200% 100%;animation:shimmer 1.4s infinite;}
@keyframes shimmer{0%{background-position:200% 0}100%{background-position:-200% 0}}
.field{margin-bottom:14px}
label{display:block;font-size:12.5px;font-weight:600;color:var(--muted);margin-bottom:6px;letter-spacing:.02em}
.input-wrap{position:relative}
input[type=text],input[type=password]{width:100%;padding:12px 14px;border-radius:12px;
  border:1px solid var(--border2);background:#0b0d14;color:var(--text);font-size:14.5px;
  outline:none;transition:border-color .18s,box-shadow .18s;}
input:focus{border-color:var(--accent);box-shadow:0 0 0 3px #4f8eff22;}
.eye-btn{position:absolute;right:12px;top:50%;transform:translateY(-50%);
  background:none;border:none;color:var(--muted);cursor:pointer;padding:4px;font-size:16px;line-height:1;}
.divider{height:1px;background:var(--border);margin:18px 0}
#btn-connect{width:100%;padding:13px;border-radius:13px;border:none;
  background:linear-gradient(135deg,var(--accent),var(--accent2));
  color:#fff;font-size:15px;font-weight:700;cursor:pointer;
  transition:opacity .18s,transform .12s;box-shadow:0 4px 20px #4f8eff40;}
#btn-connect:hover{opacity:.9;transform:translateY(-1px)}
#btn-connect:active{transform:scale(.98)}
#btn-connect:disabled{opacity:.45;cursor:not-allowed;transform:none}
#btn-scan{display:flex;align-items:center;gap:6px;background:none;
  border:1px solid var(--border2);color:var(--muted);font-size:12px;font-weight:600;
  border-radius:8px;padding:5px 10px;cursor:pointer;transition:border-color .18s,color .18s;}
#btn-scan:hover{border-color:var(--accent);color:var(--accent)}
#btn-scan.spin svg{animation:spin .9s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}
#toast{position:fixed;bottom:24px;left:50%;transform:translateX(-50%) translateY(30px);
  background:#1e2030;border:1px solid var(--border2);color:var(--text);
  padding:11px 20px;border-radius:40px;font-size:13px;font-weight:600;
  box-shadow:0 8px 32px #0008;opacity:0;transition:opacity .28s,transform .28s;
  pointer-events:none;white-space:nowrap;}
#toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
#toast.success{border-color:var(--success);color:var(--success)}
#toast.error{border-color:var(--error);color:var(--error)}
#overlay{position:fixed;inset:0;background:#09090fee;display:none;
  place-items:center;z-index:99;flex-direction:column;gap:18px;}
#overlay.show{display:flex}
.spinner{width:48px;height:48px;border-radius:50%;
  border:3px solid var(--border2);border-top-color:var(--accent);
  animation:spin .8s linear infinite;}
#overlay p{color:var(--muted);font-size:14px;font-weight:500}
</style>
</head>
<body>
<div class="wrap">
  <div class="header">
    <div class="logo-ring">📡</div>
    <h1>Wi-Fi Setup</h1>
    <p>Select your network and connect</p>
  </div>
  <div class="card">
    <div class="section-title" style="justify-content:space-between">
      <span>Available Networks</span>
      <button id="btn-scan" onclick="doScan()" title="Refresh">
        <svg id="scan-icon" width="13" height="13" viewBox="0 0 24 24" fill="none"
             stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
          <path d="M23 4v6h-6"/><path d="M1 20v-6h6"/>
          <path d="M3.51 9a9 9 0 0 1 14.36-3.36L23 10"/>
          <path d="M20.49 15a9 9 0 0 1-14.36 3.36L1 14"/>
        </svg>
        Refresh
      </button>
    </div>
    <div id="net-list">
      <div class="skeleton"></div>
      <div class="skeleton" style="opacity:.6"></div>
      <div class="skeleton" style="opacity:.35"></div>
    </div>
    <div class="divider"></div>
    <div class="section-title">Credentials</div>
    <div class="field">
      <label>Network Name (SSID)</label>
      <input id="ssid" type="text" placeholder="Tap a network or enter manually" autocomplete="off"/>
    </div>
    <div class="field">
      <label>Password</label>
      <div class="input-wrap">
        <input id="pass" type="password" placeholder="Enter password" autocomplete="new-password"/>
        <button class="eye-btn" onclick="togglePass(this)" type="button">👁</button>
      </div>
    </div>
    <button id="btn-connect" onclick="doConnect()">Connect</button>
  </div>
</div>
<div id="overlay">
  <div class="spinner"></div>
  <p id="overlay-msg">Connecting to <strong id="overlay-ssid"></strong>…</p>
</div>
<div id="toast"></div>
<script>
const $=id=>document.getElementById(id);
function toast(msg,type='',dur=3000){
  const t=$('toast');t.textContent=msg;t.className='show '+type;
  clearTimeout(t._t);t._t=setTimeout(()=>t.className='',dur);
}
function rssiToBars(r){return r>=-55?4:r>=-67?3:r>=-80?2:1;}
function barsHtml(n){
  const h=[5,8,12,16];
  return '<div class="bars">'+h.map((v,i)=>`<div class="bar${i<n?' on':''}" style="height:${v}px"></div>`).join('')+'</div>';
}
function togglePass(btn){
  const i=$('pass');i.type=i.type==='password'?'text':'password';
  btn.textContent=i.type==='password'?'👁':'🙈';
}
let scanTimer=null,selectedSsid=null;
function doScan(){
  clearTimeout(scanTimer);
  const b=$('btn-scan');b.classList.add('spin');
  fetch('/scan',{method:'POST'}).then(()=>pollScan())
    .catch(()=>{toast('Scan failed','error');b.classList.remove('spin');});
}
function pollScan(){
  fetch('/scan').then(r=>r.json()).then(d=>{
    if(d.scanning){scanTimer=setTimeout(pollScan,800);return;}
    $('btn-scan').classList.remove('spin');
    renderNetworks(d.networks||[]);
  }).catch(()=>{
    $('btn-scan').classList.remove('spin');
    $('net-list').innerHTML='<div style="color:var(--muted);font-size:13px;text-align:center;padding:16px">Could not load networks</div>';
  });
}
function renderNetworks(nets){
  const list=$('net-list');
  if(!nets.length){list.innerHTML='<div style="color:var(--muted);font-size:13px;text-align:center;padding:16px">No networks found</div>';return;}
  nets.sort((a,b)=>b.rssi-a.rssi);
  list.innerHTML=nets.map(n=>{
    const bars=rssiToBars(n.rssi),enc=n.enc?'🔒':'🌐';
    const esc=n.ssid.replace(/"/g,'&quot;').replace(/</g,'&lt;');
    const sel=n.ssid===selectedSsid?' selected':'';
    return `<div class="net-item${sel}" onclick="selectNet(this,'${esc.replace(/'/g,"\\'")}')">
      ${barsHtml(bars)}
      <div class="net-info"><div class="net-ssid">${esc}</div><div class="net-sub">${n.rssi} dBm</div></div>
      <span class="net-lock">${enc}</span></div>`;
  }).join('');
}
function selectNet(el,ssid){
  document.querySelectorAll('.net-item').forEach(i=>i.classList.remove('selected'));
  el.classList.add('selected');selectedSsid=ssid;$('ssid').value=ssid;$('pass').focus();
}
function doConnect(){
  const ssid=$('ssid').value.trim(),pass=$('pass').value;
  if(!ssid){toast('Please enter or select a network','error');return;}
  $('overlay-ssid').textContent=ssid;$('overlay').className='show';
  $('btn-connect').disabled=true;
  const fd=new FormData();fd.append('ssid',ssid);fd.append('pass',pass);
  fetch('/save',{method:'POST',body:fd}).then(r=>r.json()).then(d=>{
    if(d.ok){toast('Saved! Connecting…','success',5000);
      setTimeout(()=>{$('overlay').className='';$('btn-connect').disabled=false;},8000);}
    else throw new Error(d.error||'Unknown error');
  }).catch(err=>{
    $('overlay').className='';$('btn-connect').disabled=false;
    toast('Error: '+err.message,'error');
  });
}
window.addEventListener('DOMContentLoaded',()=>pollScan());
</script>
</body></html>)rawhtml";
  return h;
}

} // namespace atg

// ═══════════════════════════════════════════════════════════════════
//  Stub — unsupported platform
// ═══════════════════════════════════════════════════════════════════
#else

namespace atg {
WiFiPortal::WiFiPortal(const char* p,const char* a,uint32_t t,uint32_t c)
  :_productName(p),_apPass(a),_portalTimeoutMs(t),_connectTimeoutMs(c){}
Result WiFiPortal::begin(Runtime& rt){(void)rt;
  Log::e(F("ATG_WiFiPortal"),F("Unsupported platform (ESP32/ESP8266 only)"));
  return Result::Error;}
void WiFiPortal::tick(Runtime& rt)                        {(void)rt;}
void WiFiPortal::resetCredentials()                       {}
void WiFiPortal::startPortal()                            {}
void WiFiPortal::stopPortal()                             {}
bool WiFiPortal::loadCredentials(String& s,String& p)     {s="";p="";return false;}
void WiFiPortal::saveCredentials(const String&,const String&){}
void WiFiPortal::tryConnect(const String&,const String&)  {}
void WiFiPortal::setupServer()                            {}
String WiFiPortal::buildScanJson()                        {return "{}";}
String WiFiPortal::uiIndexHtml() const                    {return "";}
} // namespace atg

#endif