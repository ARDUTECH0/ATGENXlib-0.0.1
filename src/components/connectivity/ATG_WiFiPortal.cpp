#include "ATG_WiFiPortal.h"

#if defined(ESP32)

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

namespace atg {

static WebServer server(80);
static DNSServer dns;
static Preferences prefs;

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
    Log::w(F("ATG_WiFiPortal"), "AP password < 8 chars — AP will be OPEN");
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);

  String ssid, pass;
  if (loadCredentials(ssid, pass)) {
    Log::i(F("ATG_WiFiPortal"), String("Saved SSID: ") + ssid + " — connecting...");
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

  // ── Connected ──────────────────────────────────────────
  if (WiFi.status() == WL_CONNECTED) {
    if (!_connected) {
      _connected  = true;
      _connecting = false;
      _ip = WiFi.localIP().toString();
      Log::i(F("ATG_WiFiPortal"), String("✔ Connected  IP: ") + _ip);
      if (_onConnected) _onConnected();
    }
    return;
  }

  // ── Was connected, now dropped ─────────────────────────
  if (_connected) {
    _connected = false;
    Log::w(F("ATG_WiFiPortal"), "Connection lost");
    if (_onDisconnected) _onDisconnected();
  }

  // ── Waiting for connection result ──────────────────────
  if (_connecting && !_portalRunning) {
    if (_connectTimeoutMs > 0 && elapsed(_connectStart, _connectTimeoutMs)) {
      Log::w(F("ATG_WiFiPortal"), "Connect timeout — starting portal");
      _connecting = false;
      startPortal();
    }
    return;
  }

  // ── Portal loop ────────────────────────────────────────
  if (_portalRunning) {
    dns.processNextRequest();
    server.handleClient();

    if (_portalTimeoutMs > 0 && elapsed(_portalStart, _portalTimeoutMs)) {
      Log::i(F("ATG_WiFiPortal"), "Portal timeout — stopping");
      stopPortal();
    }
  }
}

// ═══════════════════════════════════════════════════════════════════
//  startPortal
// ═══════════════════════════════════════════════════════════════════
void WiFiPortal::startPortal() {
  if (_portalRunning) return;

  uint32_t chip = (uint32_t)(ESP.getEfuseMac() >> 16);
  char apName[64];
  snprintf(apName, sizeof(apName), "%s-Setup-%06X", _productName, chip & 0xFFFFFF);

  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP_STA);

  // Start scan immediately so results are ready when portal opens
  WiFi.scanNetworks(true /*async*/);

  const char* pwd = (_apPass && strlen(_apPass) >= 8) ? _apPass : nullptr;
  WiFi.softAP(apName, pwd);

  IPAddress ip = WiFi.softAPIP();
  Log::i(F("ATG_WiFiPortal"), String("AP: ") + apName + "  IP: " + ip.toString());

  dns.start(53, "*", ip);
  server.stop();   // clean slate if re-entering
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
  Log::i(F("ATG_WiFiPortal"), "Portal stopped");
}

// ═══════════════════════════════════════════════════════════════════
//  Credentials
// ═══════════════════════════════════════════════════════════════════
void WiFiPortal::resetCredentials() {
  prefs.begin("atg_wifi", false);
  prefs.remove("ssid");
  prefs.remove("pass");
  prefs.end();
  Log::i(F("ATG_WiFiPortal"), "Credentials cleared");
}

bool WiFiPortal::loadCredentials(String& ssid, String& pass) {
  prefs.begin("atg_wifi", true);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
  return ssid.length() > 0;
}

void WiFiPortal::saveCredentials(const String& ssid, const String& pass) {
  prefs.begin("atg_wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
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
//  buildScanJson  — returns JSON array of visible networks
// ═══════════════════════════════════════════════════════════════════
String WiFiPortal::buildScanJson() {
  int n = WiFi.scanComplete();

  if (n == WIFI_SCAN_RUNNING) {
    return "{\"scanning\":true,\"networks\":[]}";
  }

  if (n <= 0) {
    WiFi.scanNetworks(true);   // retry
    return "{\"scanning\":true,\"networks\":[]}";
  }

  String json = "{\"scanning\":false,\"networks\":[";
  for (int i = 0; i < n; i++) {
    if (i) json += ',';
    String ess = WiFi.SSID(i);
    ess.replace("\\", "\\\\");
    ess.replace("\"", "\\\"");
    json += '{';
    json += "\"ssid\":\"" + ess + "\",";
    json += "\"rssi\":"   + String(WiFi.RSSI(i)) + ',';
    json += "\"enc\":"    + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? 1 : 0);
    json += '}';
  }
  json += "]}";

  WiFi.scanDelete();
  WiFi.scanNetworks(true);   // keep refreshing
  return json;
}

// ═══════════════════════════════════════════════════════════════════
//  setupServer
// ═══════════════════════════════════════════════════════════════════
void WiFiPortal::setupServer() {

  // Main page
  server.on("/", HTTP_GET, [&]() {
    server.send(200, "text/html; charset=utf-8", uiIndexHtml());
  });

  // WiFi scan JSON  GET /scan
  server.on("/scan", HTTP_GET, [&]() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", buildScanJson());
  });

  // Rescan trigger  POST /scan
  server.on("/scan", HTTP_POST, [&]() {
    WiFi.scanNetworks(true);
    server.send(200, "application/json", "{\"ok\":true}");
  });

  // Save credentials  POST /save
  server.on("/save", HTTP_POST, [&]() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

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

  // Captive-portal redirect catch-all
  server.onNotFound([&]() {
    String host = server.hostHeader();
    if (host.length() && host != WiFi.softAPIP().toString()) {
      server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
      server.send(302, "text/plain", "");
    } else {
      server.send(200, "text/html; charset=utf-8", uiIndexHtml());
    }
  });

  server.begin();
}

// ═══════════════════════════════════════════════════════════════════
//  uiIndexHtml  — full single-page WiFi Manager UI
// ═══════════════════════════════════════════════════════════════════
String WiFiPortal::uiIndexHtml() const {
  // ~6 KB — stored in DRAM; large enough to justify a reserve
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

/* ── Tokens ── */
:root{
  --bg:#09090f;
  --surface:#0f1117;
  --surface2:#161820;
  --border:#ffffff0d;
  --border2:#ffffff18;
  --accent:#4f8eff;
  --accent2:#7b5ea7;
  --success:#22c55e;
  --error:#ef4444;
  --text:#f1f5f9;
  --muted:#64748b;
  --card-shadow:0 2px 24px #0006,0 1px 2px #0003;
}

body{
  background:var(--bg);
  color:var(--text);
  font-family:'Segoe UI',system-ui,sans-serif;
  min-height:100vh;
  display:flex;
  align-items:flex-start;
  justify-content:center;
  padding:24px 16px 48px;
}

/* ── Background mesh ── */
body::before{
  content:'';
  position:fixed;inset:0;
  background:
    radial-gradient(ellipse 80% 50% at 20% -10%,#4f8eff18 0%,transparent 60%),
    radial-gradient(ellipse 60% 40% at 90% 110%,#7b5ea720 0%,transparent 60%);
  pointer-events:none;
  z-index:0;
}

.wrap{position:relative;z-index:1;width:100%;max-width:440px}

/* ── Header ── */
.header{text-align:center;margin-bottom:28px;padding-top:8px}
.logo-ring{
  width:56px;height:56px;border-radius:16px;
  background:linear-gradient(135deg,var(--accent),var(--accent2));
  display:flex;align-items:center;justify-content:center;
  margin:0 auto 14px;
  box-shadow:0 0 32px #4f8eff44;
  font-size:22px;
}
.header h1{font-size:22px;font-weight:700;letter-spacing:-.3px}
.header p{margin-top:4px;color:var(--muted);font-size:13.5px}

/* ── Card ── */
.card{
  background:var(--surface);
  border:1px solid var(--border);
  border-radius:20px;
  padding:22px;
  box-shadow:var(--card-shadow);
  backdrop-filter:blur(12px);
}

/* ── Section title ── */
.section-title{
  font-size:11px;font-weight:600;letter-spacing:.08em;
  text-transform:uppercase;color:var(--muted);
  margin-bottom:10px;display:flex;align-items:center;gap:8px;
}
.section-title::after{content:'';flex:1;height:1px;background:var(--border2)}

/* ── Network list ── */
#net-list{
  display:flex;flex-direction:column;gap:6px;
  margin-bottom:18px;min-height:56px;
}
.net-item{
  display:flex;align-items:center;gap:12px;
  padding:12px 14px;border-radius:13px;
  background:var(--surface2);border:1px solid var(--border);
  cursor:pointer;transition:border-color .18s,background .18s;
  user-select:none;
}
.net-item:hover{border-color:var(--accent);background:#4f8eff0a}
.net-item.selected{border-color:var(--accent);background:#4f8eff12}
.net-icon{flex-shrink:0;color:var(--accent)}
.net-info{flex:1;min-width:0}
.net-ssid{font-size:14px;font-weight:600;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.net-sub{font-size:11.5px;color:var(--muted);margin-top:1px}
.net-lock{color:var(--muted);flex-shrink:0;font-size:13px}

/* Signal bars */
.bars{display:flex;align-items:flex-end;gap:2px;height:16px;flex-shrink:0}
.bar{width:4px;border-radius:2px;background:var(--border2)}
.bar.on{background:var(--accent)}

/* ── Skeleton ── */
.skeleton{
  height:52px;border-radius:13px;
  background:linear-gradient(90deg,var(--surface2) 25%,#ffffff07 50%,var(--surface2) 75%);
  background-size:200% 100%;
  animation:shimmer 1.4s infinite;
}
@keyframes shimmer{0%{background-position:200% 0}100%{background-position:-200% 0}}

/* ── Inputs ── */
.field{margin-bottom:14px}
label{display:block;font-size:12.5px;font-weight:600;color:var(--muted);margin-bottom:6px;letter-spacing:.02em}
.input-wrap{position:relative}
input[type=text],input[type=password]{
  width:100%;padding:12px 14px;
  border-radius:12px;
  border:1px solid var(--border2);
  background:#0b0d14;
  color:var(--text);
  font-size:14.5px;
  outline:none;
  transition:border-color .18s,box-shadow .18s;
}
input:focus{
  border-color:var(--accent);
  box-shadow:0 0 0 3px #4f8eff22;
}
.eye-btn{
  position:absolute;right:12px;top:50%;transform:translateY(-50%);
  background:none;border:none;color:var(--muted);cursor:pointer;
  padding:4px;font-size:16px;line-height:1;
}

/* ── Divider ── */
.divider{height:1px;background:var(--border);margin:18px 0}

/* ── Submit button ── */
#btn-connect{
  width:100%;padding:13px;
  border-radius:13px;border:none;
  background:linear-gradient(135deg,var(--accent),var(--accent2));
  color:#fff;font-size:15px;font-weight:700;
  cursor:pointer;letter-spacing:.01em;
  transition:opacity .18s,transform .12s,box-shadow .18s;
  box-shadow:0 4px 20px #4f8eff40;
}
#btn-connect:hover{opacity:.9;transform:translateY(-1px)}
#btn-connect:active{transform:scale(.98)}
#btn-connect:disabled{opacity:.45;cursor:not-allowed;transform:none}

/* ── Rescan ── */
#btn-scan{
  display:flex;align-items:center;gap:6px;
  background:none;border:1px solid var(--border2);
  color:var(--muted);font-size:12px;font-weight:600;
  border-radius:8px;padding:5px 10px;cursor:pointer;
  transition:border-color .18s,color .18s;
}
#btn-scan:hover{border-color:var(--accent);color:var(--accent)}
#btn-scan.spin svg{animation:spin .9s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}

/* ── Toast ── */
#toast{
  position:fixed;bottom:24px;left:50%;transform:translateX(-50%) translateY(30px);
  background:#1e2030;border:1px solid var(--border2);
  color:var(--text);padding:11px 20px;
  border-radius:40px;font-size:13px;font-weight:600;
  box-shadow:0 8px 32px #0008;
  opacity:0;transition:opacity .28s,transform .28s;pointer-events:none;
  white-space:nowrap;
}
#toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
#toast.success{border-color:var(--success);color:var(--success)}
#toast.error{border-color:var(--error);color:var(--error)}

/* ── Connecting overlay ── */
#overlay{
  position:fixed;inset:0;background:#09090fee;
  display:none;place-items:center;z-index:99;
  flex-direction:column;gap:18px;
}
#overlay.show{display:flex}
.spinner{
  width:48px;height:48px;border-radius:50%;
  border:3px solid var(--border2);
  border-top-color:var(--accent);
  animation:spin .8s linear infinite;
}
#overlay p{color:var(--muted);font-size:14px;font-weight:500}
</style>
</head>
<body>
<div class="wrap">

  <!-- Header -->
  <div class="header">
    <div class="logo-ring">📡</div>
    <h1>Wi-Fi Setup</h1>
    <p>Select your network and connect</p>
  </div>

  <!-- Card -->
  <div class="card">

    <!-- Networks -->
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

    <!-- Credentials -->
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

  </div><!-- /card -->
</div><!-- /wrap -->

<!-- Connecting overlay -->
<div id="overlay">
  <div class="spinner"></div>
  <p id="overlay-msg">Connecting to <strong id="overlay-ssid"></strong>…</p>
</div>

<!-- Toast -->
<div id="toast"></div>

<script>
// ── Helpers ───────────────────────────────────────────────────────
const $  = id => document.getElementById(id);
const qs = s  => document.querySelector(s);

function toast(msg, type='', dur=3000) {
  const t = $('toast');
  t.textContent = msg;
  t.className = 'show ' + type;
  clearTimeout(t._t);
  t._t = setTimeout(() => t.className = '', dur);
}

function rssiToBars(rssi) {
  if (rssi >= -55) return 4;
  if (rssi >= -67) return 3;
  if (rssi >= -80) return 2;
  return 1;
}

function barsHtml(n) {
  const heights = [5, 8, 12, 16];
  return '<div class="bars">' +
    heights.map((h,i) =>
      `<div class="bar${i<n?' on':''}" style="height:${h}px"></div>`
    ).join('') + '</div>';
}

function togglePass(btn) {
  const inp = $('pass');
  inp.type = inp.type === 'password' ? 'text' : 'password';
  btn.textContent = inp.type === 'password' ? '👁' : '🙈';
}

// ── Scan ─────────────────────────────────────────────────────────
let scanTimer = null;
let selectedSsid = null;

function doScan() {
  clearTimeout(scanTimer);
  const btn = $('btn-scan');
  btn.classList.add('spin');
  fetch('/scan', {method:'POST'})
    .then(() => { pollScan(); })
    .catch(() => { toast('Scan failed','error'); btn.classList.remove('spin'); });
}

function pollScan() {
  fetch('/scan')
    .then(r => r.json())
    .then(data => {
      if (data.scanning) {
        scanTimer = setTimeout(pollScan, 800);
        return;
      }
      $('btn-scan').classList.remove('spin');
      renderNetworks(data.networks || []);
    })
    .catch(() => {
      $('btn-scan').classList.remove('spin');
      $('net-list').innerHTML = '<div style="color:var(--muted);font-size:13px;text-align:center;padding:16px">Could not load networks</div>';
    });
}

function renderNetworks(nets) {
  const list = $('net-list');
  if (!nets.length) {
    list.innerHTML = '<div style="color:var(--muted);font-size:13px;text-align:center;padding:16px">No networks found</div>';
    return;
  }
  // Sort by RSSI desc
  nets.sort((a,b) => b.rssi - a.rssi);

  list.innerHTML = nets.map(n => {
    const bars = rssiToBars(n.rssi);
    const enc  = n.enc ? '🔒' : '🌐';
    const dbm  = n.rssi + ' dBm';
    const esc  = n.ssid.replace(/"/g,'&quot;').replace(/</g,'&lt;');
    const sel  = n.ssid === selectedSsid ? ' selected' : '';
    return `<div class="net-item${sel}" onclick="selectNet(this,'${esc.replace(/'/g,"\\'")}')">
      ${barsHtml(bars)}
      <div class="net-info">
        <div class="net-ssid">${esc}</div>
        <div class="net-sub">${dbm}</div>
      </div>
      <span class="net-lock">${enc}</span>
    </div>`;
  }).join('');
}

function selectNet(el, ssid) {
  document.querySelectorAll('.net-item').forEach(i => i.classList.remove('selected'));
  el.classList.add('selected');
  selectedSsid = ssid;
  $('ssid').value = ssid;
  $('pass').focus();
}

// ── Connect ───────────────────────────────────────────────────────
function doConnect() {
  const ssid = $('ssid').value.trim();
  const pass = $('pass').value;

  if (!ssid) { toast('Please enter or select a network','error'); return; }

  // Show overlay
  $('overlay-ssid').textContent = ssid;
  $('overlay').className = 'show';
  $('btn-connect').disabled = true;

  const fd = new FormData();
  fd.append('ssid', ssid);
  fd.append('pass', pass);

  fetch('/save', {method:'POST', body: fd})
    .then(r => r.json())
    .then(data => {
      if (data.ok) {
        toast('Saved! Connecting…', 'success', 5000);
        setTimeout(() => { $('overlay').className=''; $('btn-connect').disabled=false; }, 8000);
      } else {
        throw new Error(data.error || 'Unknown error');
      }
    })
    .catch(err => {
      $('overlay').className = '';
      $('btn-connect').disabled = false;
      toast('Error: ' + err.message, 'error');
    });
}

// ── Boot ──────────────────────────────────────────────────────────
window.addEventListener('DOMContentLoaded', () => {
  pollScan();
});
</script>
</body>
</html>)rawhtml";

  return h;
}

} // namespace atg

// ═══════════════════════════════════════════════════════════════════
//  Stub build (disabled platform)
// ═══════════════════════════════════════════════════════════════════
#else
namespace atg {

WiFiPortal::WiFiPortal(const char* p, const char* a, uint32_t t, uint32_t c)
  : _productName(p), _apPass(a), _portalTimeoutMs(t), _connectTimeoutMs(c) {}

Result WiFiPortal::begin(Runtime& rt) {
  (void)rt;
  Log::e(F("ATG_WiFiPortal"),
    "Disabled. Enable ATG_ENABLE_WIFI=1, ATG_ENABLE_WIFI_PORTAL=1 (ESP32 only).");
  return Result::Error;
}
void WiFiPortal::tick(Runtime& rt)              { (void)rt; }
void WiFiPortal::resetCredentials()             {}
void WiFiPortal::startPortal()                  {}
void WiFiPortal::stopPortal()                   {}
bool WiFiPortal::loadCredentials(String& s, String& p) { s=""; p=""; return false; }
void WiFiPortal::saveCredentials(const String&, const String&) {}
void WiFiPortal::tryConnect(const String&, const String&)      {}
void WiFiPortal::setupServer()                  {}
String WiFiPortal::buildScanJson()              { return "{}"; }
String WiFiPortal::uiIndexHtml()  const         { return ""; }

} // namespace atg
#endif