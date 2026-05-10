#pragma once

#include "platform_compat.h"
#include "config.h"

extern ClockWebServer server;
extern DNSServer        dnsServer;

// ============================================================
// Web-UI: HTML-Hilfsfunktionen (Streaming)
// ============================================================
static void chunk(const String& s)              { server.sendContent(s); yield(); }
static void chunk(const __FlashStringHelper* s) { server.sendContent(s); yield(); }

// ============================================================
// HTML-Escape / Form-Helpers
// ============================================================
static String htmlEscape(String s) {
  s.replace(F("&"),  F("&amp;"));
  s.replace(F("<"),  F("&lt;"));
  s.replace(F(">"),  F("&gt;"));
  s.replace(F("\""), F("&quot;"));
  s.replace(F("'"),  F("&#39;"));
  return s;
}
static String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 8);
  for (uint16_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if      (c == '"')  out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') {}
    else if ((uint8_t)c < 0x20) {}
    else                out += c;
  }
  return out;
}
static const char* checked(bool v)  { return v ? " checked"  : ""; }
static const char* selected(bool v) { return v ? " selected" : ""; }

static const char* dstModeLabel(uint8_t m) {
  switch (m) {
    case 0:  return "Aus";
    case 1:  return "EU (letzter So. Mär/Okt)";
    case 2:  return "US (2. So. Mär / 1. So. Nov)";
    default: return "Benutzerdefiniert (M.w.d/h)";
  }
}

// ============================================================
// Web-UI: handleRoot() — modernes Tab-Design
// ============================================================
static void handleRoot() {
  bool inAP = WiFi.getMode() & WIFI_AP;
  String lux_s = isfinite(luxHold) ? (String(luxHold, 1) + " lx") : String("-");
  String bl_s = String((int)displayIntensityApplied) + " / 15";
  String sensor_s = vemlOk ? F("OK") : vemlDiag;
  String auto_s = cfg.auto_brightness ? F("aktiv") : F("aus");

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("text/html"), "");

  // ---- HEAD & CSS ----
  chunk(F("<!doctype html><html lang='de'><head>"
          "<meta charset='utf-8'>"
          "<meta name='viewport' content='width=device-width,initial-scale=1'>"
          "<title>Dottie - The Matrix Clock</title>"
          "<style>"
          ":root{--c:#3b82f6;--bg:#f8fafc;--card:#fff;--border:#e2e8f0;--text:#1e293b;--muted:#64748b}"
          "body{font-family:system-ui,Arial,sans-serif;margin:0;background:var(--bg);color:var(--text)}"
          ".hdr{background:var(--c);color:#fff;padding:10px 16px;display:flex;align-items:center;gap:10px;flex-wrap:wrap}"
          ".hdr h1{margin:0;font-size:1.2rem;flex:1}"
          ".hdr h1{margin:0;font-size:1.3rem}"
          ".badge{background:rgba(255,255,255,.2);border-radius:6px;padding:2px 8px;font-size:.8rem}"
          ".wrap{max-width:860px;margin:auto;padding:16px}"
          ".tabs{display:flex;gap:4px;margin-bottom:16px;flex-wrap:wrap}"
          ".tab{padding:8px 16px;border-radius:8px 8px 0 0;cursor:pointer;background:var(--border);"
               "border:1px solid var(--border);border-bottom:none;font-size:.9rem}"
          ".tab.active{background:var(--card);border-color:var(--border);color:var(--c);font-weight:600}"
          ".panel{display:none;background:var(--card);border:1px solid var(--border);"
                 "border-radius:0 8px 8px 8px;padding:20px}"
          ".panel.active{display:block}"
          ".row2{display:grid;grid-template-columns:1fr 1fr;gap:14px}"
          ".row3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:14px}"
          "label{display:block;font-size:.85rem;font-weight:600;margin:10px 0 3px;color:var(--muted)}"
          "input,select{width:100%;padding:8px 10px;border:1px solid var(--border);"
                       "border-radius:6px;font-size:.95rem;box-sizing:border-box}"
          "input:focus,select:focus{outline:2px solid var(--c)}"
          "input:disabled{opacity:.45}"
          ".chk{display:flex;align-items:center;gap:8px;margin:10px 0}"
          ".chk input{width:auto}"
          "button{padding:9px 16px;font-size:.95rem;border-radius:6px;border:none;"
                 "cursor:pointer;background:var(--c);color:#fff}"
          "button:hover{filter:brightness(1.1)}"
          ".btn-danger{background:#ef4444}"
          ".btn-sec{background:var(--muted)}"
          ".stat{background:var(--bg);border-radius:8px;padding:12px;margin:8px 0;font-size:.9rem}"
          ".stat b{color:var(--c)}"
          ".ok{color:#16a34a} .fail{color:#dc2626}"
          ".mono{font-family:monospace;font-size:.82rem;white-space:pre-wrap;background:var(--bg);"
                "padding:10px;border-radius:6px;margin:6px 0}"
          ".hint{font-size:.8rem;color:var(--muted);margin-top:3px}"
          "h3{margin:0 0 12px;font-size:1rem;border-bottom:1px solid var(--border);padding-bottom:6px}"
          "@media(max-width:600px){.row2,.row3{grid-template-columns:1fr}}"
          ".dm-panel{background:#0a0a0a;border:1px solid #1e1e2a;border-radius:10px;padding:10px 14px 8px;margin-left:auto;box-shadow:0 0 24px rgba(255,140,0,.08),inset 0 0 20px rgba(0,0,0,.6);}"
          ".dm-lbl{font-size:8px;letter-spacing:2px;color:#2a2a3a;text-align:center;text-transform:uppercase;margin-bottom:5px;font-family:monospace;}"
          ".dm-div{height:1px;background:linear-gradient(to right,transparent,#1a1a2a,transparent);margin:5px 0;}"
          "</style>"
          "<script>"
          "function showTab(id){"
          "document.querySelectorAll('.tab,.panel').forEach(e=>e.classList.remove('active'));"
          "document.getElementById('t_'+id).classList.add('active');"
          "document.getElementById('p_'+id).classList.add('active');"
          "localStorage.setItem('tab',id);}"
          "window.onload=function(){"
          "var t=localStorage.getItem('tab')||'status';"
          "showTab(t);"
          "updDhcp();"
          "updDst();"
          "};"
          "function updDhcp(){"
          "var dh=document.getElementById('use_dhcp');"
          "if(!dh)return;"
          "var on=dh.checked;"
          "['static_ip','static_gw','static_mask','dns1','dns2'].forEach(function(id){"
          "var e=document.getElementById(id);if(!e)return;"
          "e.disabled=on;e.style.opacity=on?0.4:1;});}"
          "function updDst(){"
          "var sel=document.getElementById('dst_mode');"
          "if(!sel)return;"
          "var show=sel.value=='3';"
          "var sec=document.getElementById('dst_custom');"
          "if(sec)sec.style.display=show?'block':'none';}"
                    "var F8={"
          "'0':[0x3C,0x66,0x66,0x66,0x66,0x66,0x66,0x3C],"
          "'1':[0x18,0x38,0x18,0x18,0x18,0x18,0x18,0x7E],"
          "'2':[0x3C,0x66,0x06,0x0C,0x18,0x30,0x60,0x7E],"
          "'3':[0x3C,0x66,0x06,0x1C,0x06,0x06,0x66,0x3C],"
          "'4':[0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x0C],"
          "'5':[0x7E,0x60,0x60,0x7C,0x06,0x06,0x66,0x3C],"
          "'6':[0x1C,0x30,0x60,0x7C,0x66,0x66,0x66,0x3C],"
          "'7':[0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x30],"
          "'8':[0x3C,0x66,0x66,0x3C,0x66,0x66,0x66,0x3C],"
          "'9':[0x3C,0x66,0x66,0x66,0x3E,0x06,0x0C,0x38],"
          "'\u003A':[0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00],"
          "' ':[0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00]};"
          "var F5={"
          "'0':[0x3E,0x51,0x49,0x45,0x3E],"
          "'1':[0x00,0x42,0x7F,0x40,0x00],"
          "'2':[0x42,0x61,0x51,0x49,0x46],"
          "'3':[0x21,0x41,0x45,0x4B,0x31],"
          "'4':[0x18,0x14,0x12,0x7F,0x10],"
          "'5':[0x27,0x45,0x45,0x45,0x39],"
          "'6':[0x3C,0x4A,0x49,0x49,0x30],"
          "'7':[0x01,0x71,0x09,0x05,0x03],"
          "'8':[0x36,0x49,0x49,0x49,0x36],"
          "'9':[0x06,0x49,0x49,0x29,0x1E],"
          "'\u002E':[0x00,0x00,0x60,0x60,0x00],"
          "' ':[0x00,0x00,0x00,0x00,0x00]};"
          "function dF8(cv,txt,dp,gp,cOn,cOff,gw){"
          "var st=dp+gp,R=8,C=8,cw=C*st,sw=st;"
          "cv.width=Math.ceil(txt.length*cw+Math.max(0,txt.length-1)*sw);"
          "cv.height=Math.ceil(R*st);"
          "var ctx=cv.getContext('2d');"
          "ctx.clearRect(0,0,cv.width,cv.height);"
          "for(var ci=0;ci<txt.length;ci++){"
          "var g=F8[txt[ci]]||F8[' '],ox=ci*(cw+sw);"
          "for(var row=0;row<R;row++){"
          "var rb=g[row];"
          "for(var col=0;col<C;col++){"
          "var on=(rb>>(7-col))&1;"
          "ctx.beginPath();"
          "ctx.arc(ox+col*st+dp/2,row*st+dp/2,dp/2,0,Math.PI*2);"
          "if(on){ctx.shadowColor=gw;ctx.shadowBlur=dp*3;ctx.fillStyle=cOn;}"
          "else{ctx.shadowBlur=0;ctx.fillStyle=cOff;}"
          "ctx.fill();}}}}"
          "function dF5(cv,txt,dp,gp,cOn,cOff,gw){"
          "var st=dp+gp,R=7,tc=0;"
          "for(var i=0;i<txt.length;i++){tc+=(F5[txt[i]]||F5[' ']).length;if(i<txt.length-1)tc++;}"
          "cv.width=Math.ceil(tc*st);cv.height=Math.ceil(R*st);"
          "var ctx=cv.getContext('2d'),cx=0;"
          "ctx.clearRect(0,0,cv.width,cv.height);"
          "for(var ci=0;ci<txt.length;ci++){"
          "var g=F5[txt[ci]]||F5[' '];"
          "for(var col=0;col<g.length;col++){"
          "var cb=g[col];"
          "for(var row=0;row<R;row++){"
          "var on=(cb>>row)&1;"
          "ctx.beginPath();"
          "ctx.arc((cx+col)*st+dp/2,row*st+dp/2,dp/2,0,Math.PI*2);"
          "if(on){ctx.shadowColor=gw;ctx.shadowBlur=dp*3;ctx.fillStyle=cOn;}"
          "else{ctx.shadowBlur=0;ctx.fillStyle=cOff;}"
          "ctx.fill();}"
          "}"
          "cx+=g.length+1;}"
          "}"
          "var espLocalSec=0,espFetchMs=0;"
          "function redrawClock(){"
          "if(!espFetchMs)return;"
          "var elapsed=Math.floor((Date.now()-espFetchMs)/1000);"
          "var s=(espLocalSec+elapsed)%86400;"
          "var hh=Math.floor(s/3600),mm=Math.floor((s%3600)/60),ss=s%60;"
          "var p=function(n){return String(n).padStart(2,'0');};"
          "var tstr=p(hh)+'\u003A'+p(mm)+'\u003A'+p(ss);"
          "var c1=document.getElementById('dm_time');"
          "if(c1)dF8(c1,tstr,5,1.5,'#FFA030','#1c1005','#FF8000');"
          "}"
          "function pollStatus(){"
          "fetch('/api/status')"
          ".then(function(r){return r.json();})"
          ".then(function(d){"
          "var u=document.getElementById('hdr_uptime');"
          "if(u)u.textContent=d.uptime;"
          "var ns=document.getElementById('next_sync');"
          "if(ns&&d.nextsync!==undefined)ns.textContent=d.nextsync+'s';"
          "var lux=document.getElementById('v-lux');"
          "if(lux)lux.textContent=d.lux>=0?d.lux.toFixed(1)+' lx':'-';"
          "var bl=document.getElementById('v-bl');"
          "if(bl)bl.textContent=d.brightness+' / 15';"
          "var sa=document.getElementById('v-auto');"
          "if(sa)sa.textContent=d.auto_brightness?'aktiv':'aus';"
          "var ss=document.getElementById('v-sensor');"
          "if(ss)ss.textContent=d.sensor_msg|| (d.sensor_ok?'OK':'nicht gefunden');"
          "espLocalSec=d.localsec||0;espFetchMs=Date.now();"
          "var c2=document.getElementById('dm_date');"
          "if(c2)dF5(c2,d.date,3.2,1.1,'#CC6A00','#130d03','#AA5000');"
          "redrawClock();"
          "})"
          ".catch(function(){});"
          "}"
          "setInterval(redrawClock,1000);"
          "setInterval(pollStatus,5000);"
          "pollStatus();"
          "function updPwm(v){"
          "var o=document.getElementById('pwm_out');"
          "if(o)o.value=v*10+'%';"
          "}"
          "function ntpTest(i){"
          "var box=document.getElementById('ntpTestResult');"
          "var title=document.getElementById('ntpTestTitle');"
          "var status=document.getElementById('ntpTestStatus');"
          "var detail=document.getElementById('ntpTestDetail');"
          "box.style.display='block';"
          "title.textContent='NTP #'+i+' — Test läuft...';"
          "status.innerHTML='<i>⏳ Bitte warten...</i>';"
          "detail.textContent='';"
          "fetch('/test?i='+i,{method:'POST'})"
          ".then(function(){pollNtpResult(i,0);})"
          ".catch(function(e){status.innerHTML='Fehler: '+e;});}"
          "function pollNtpResult(i,tries){"
          "if(tries>30){document.getElementById('ntpTestStatus').innerHTML='Timeout';return;}"
          "fetch('/api/ntptest')"
          ".then(function(r){return r.json();})"
          ".then(function(d){"
          "if(d.running){setTimeout(function(){pollNtpResult(i,tries+1);},500);return;}"
          "var s2=document.getElementById('ntpTestStatus');"
          "var d2=document.getElementById('ntpTestDetail');"
          "var t2=document.getElementById('ntpTestTitle');"
          "t2.textContent='NTP #'+i+' — Ergebnis';"
          "var ok=d.result==='OK';"
          "s2.innerHTML=ok?'<span style=\"color:#16a34a\">✓ Erfolgreich</span>':'<span style=\"color:#dc2626\">✗ Fehlgeschlagen</span>';"
          "d2.textContent=d.host+'\\n'+d.detail;})"
          ".catch(function(e){setTimeout(function(){pollNtpResult(i,tries+1);},500);});}"
          "</script>"
          "</head><body>"));

  // ---- HEADER ----
  chunk(F("<div class='hdr'>"
          "<svg width='28' height='28' viewBox='0 0 24 24' fill='none' stroke='#fff' stroke-width='2'>"
          "<circle cx='12' cy='12' r='10'/><polyline points='12 6 12 12 16 14'/></svg>"
          "<h1>Dottie &mdash; The Matrix Clock</h1>"));
  chunk(String(F("<span class='badge'>")) + FPSTR(FW_VERSION) + F("</span>"));
  chunk(F("<span class='badge'>Uptime: <span id='hdr_uptime'>...</span></span>"));
  chunk(F("<div class='dm-panel'>"
          "<div class='dm-lbl'>NTP &middot; CLOCK</div>"
          "<canvas id='dm_time'></canvas>"
          "<div class='dm-div'></div>"
          "<canvas id='dm_date'></canvas>"
          "</div>"));
  chunk(F("</div><div class='wrap'>"));

  // ---- TABS ----
  chunk(F("<div class='tabs'>"));
  chunk(F("<div class='tab' id='t_status'  onclick='showTab(\"status\")'>📊 Status</div>"));
  chunk(F("<div class='tab' id='t_wifi'    onclick='showTab(\"wifi\")'>📶 WiFi</div>"));
  chunk(F("<div class='tab' id='t_ntp'     onclick='showTab(\"ntp\")'>🕐 NTP</div>"));
  chunk(F("<div class='tab' id='t_tz'      onclick='showTab(\"tz\")'>🌍 Zeitzone</div>"));
  chunk(F("<div class='tab' id='t_display' onclick='showTab(\"display\")'>💡 Display</div>"));
  chunk(F("<div class='tab' id='t_system'  onclick='showTab(\"system\")'>⚙️ System</div>"));
  chunk(F("</div>"));

  chunk(F("<form method='POST' action='/save'>"));

  // ============================================================
  // TAB: STATUS
  // ============================================================
  chunk(F("<div class='panel' id='p_status'>"));
  chunk(F("<h3>Gerätestatus</h3>"));

  // WiFi
  if (inAP) {
    chunk(String(F("<div class='stat'>📡 <b>Modus:</b> Access Point &nbsp; <b>AP-IP:</b> ")) +
          WiFi.softAPIP().toString() + F("</div>"));
  } else {
    bool wifiOk = (WiFi.status() == WL_CONNECTED);
    chunk(String(F("<div class='stat'>📶 <b>SSID:</b> ")) + htmlEscape(cfg.sta_ssid)
        + F(" &nbsp; <b>IP:</b> ") + WiFi.localIP().toString()
        + F(" &nbsp; <b>RSSI:</b> ") + WiFi.RSSI() + F(" dBm"
        " &nbsp; <b>Status:</b> <span class='") + (wifiOk ? F("ok'>✓ Verbunden") : F("fail'>✗ Getrennt")) + F("</span>"
        "</div>"));
    chunk(String(F("<div class='stat'>🌐 <b>DNS:</b> ")) + ipOrLabel(WiFi.dnsIP(0))
        + F(" / ") + ipOrLabel(WiFi.dnsIP(1))
        + F(" &nbsp; <b>Aktiver NTP:</b> ") + htmlEscape(activeNtp.length() ? activeNtp : "(keiner)")
        + F(" &nbsp; <b>Nächster Sync:</b> <span id='next_sync'>") + nextSyncInSec() + F("s</span>"
        "</div>"));
  }

  // Letzte Sync-Zeit
  if (lastSyncEpoch > 0) {
    char buf[32]; struct tm tm_s;
    localtime_r(&lastSyncEpoch, &tm_s);
    strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", &tm_s);
    chunk(String(F("<div class='stat'>🕐 <b>Letzter NTP-Sync:</b> ")) + buf + F("</div>"));
  }

  // NTP-Log
  if (ntpLogCount > 0) {
    chunk(F("<h3 style='margin-top:16px'>NTP-Sync-Protokoll (letzte Einträge)</h3>"));
    // Iteriere rückwärts (neueste zuerst)
    for (int8_t i = 0; i < (int8_t)ntpLogCount; i++) {
      int8_t idx = ((int8_t)ntpLogHead - 1 - i + NTP_LOG_ENTRIES) % NTP_LOG_ENTRIES;
      const NtpLogEntry& e = ntpLog[idx];
      char tbuf[20] = "--:--:--";
      if (e.timestampEpoch > 0) {
        struct tm tm_e;
        localtime_r((const time_t*)&e.timestampEpoch, &tm_e);
        strftime(tbuf, sizeof(tbuf), "%H:%M:%S", &tm_e);
      }
      chunk(String(F("<div class='mono'>")) +
            tbuf + F(" | ") +
            (e.success ? F("<span class='ok'>OK</span>") : F("<span class='fail'>FAIL</span>")) +
            F(" | ") + htmlEscape(e.host) + F("\n") + htmlEscape(e.detail) +
            F("</div>"));
    }
  }

  // Letzter manueller Test
  if (lastTestMs > 0) {
    chunk(F("<h3 style='margin-top:16px'>Letzter manueller NTP-Test</h3>"));
    chunk(String(F("<div class='stat'>")) +
          F("<b>Host:</b> ") + htmlEscape(lastTestHost) +
          F(" &nbsp; <b>Ergebnis:</b> <span class='") +
          (lastTestResult == "OK" ? F("ok'>✓ OK") : F("fail'>✗ FAIL")) + F("</span>") +
          F(" &nbsp; <b>vor:</b> ") + (millis() - lastTestMs) / 1000 + F("s</div>"));
    chunk(String(F("<div class='mono'>")) + htmlEscape(lastTestDetail) + F("</div>"));
  }

  chunk(F("</div>")); // panel status

  // ============================================================
  // TAB: WIFI
  // ============================================================
  chunk(F("<div class='panel' id='p_wifi'>"));
  chunk(F("<h3>WLAN-Zugangsdaten</h3>"));
  chunk(F("<div class='row2'>"));
  chunk(String(F("<div><label>SSID</label><input name='sta_ssid' value='")) + htmlEscape(cfg.sta_ssid) + F("'></div>"));
  chunk(String(F("<div><label>Passwort</label><input name='sta_pass' type='password' value='")) + htmlEscape(cfg.sta_pass) + F("'></div>"));
  chunk(F("</div>"));
  chunk(F("<div class='row2'>"));
  chunk(String(F("<div><label>Verbindungs-Timeout (Sek.)</label>"
                 "<input name='wifi_timeout_sec' type='number' min='5' max='600' value='")) + cfg.wifi_timeout_sec + F("'></div>"));
  chunk(String(F("<div><label>IP anzeigen nach Verbindung (Sek., 0=aus)</label>"
                 "<input name='ip_show_sec' type='number' min='0' max='60' value='")) + cfg.ip_show_sec + F("'></div>"));
  chunk(F("</div>"));

  chunk(F("<h3>IP-Konfiguration</h3>"));
  chunk(String(F("<div class='chk'><input id='use_dhcp' type='checkbox' name='use_dhcp' value='1'")) +
        checked(cfg.use_dhcp) + F(" onchange='updDhcp()'><label style='margin:0'>DHCP verwenden</label></div>"));
  chunk(F("<div class='row3'>"));
  chunk(String(F("<div><label>Statische IP</label><input id='static_ip' name='static_ip' value='")) + htmlEscape(cfg.static_ip) + F("'></div>"));
  chunk(String(F("<div><label>Gateway</label><input id='static_gw' name='static_gw' value='")) + htmlEscape(cfg.static_gw) + F("'></div>"));
  chunk(String(F("<div><label>Subnetzmaske</label><input id='static_mask' name='static_mask' value='")) + htmlEscape(cfg.static_mask) + F("'></div>"));
  chunk(F("</div>"));
  chunk(F("<h3>DNS-Überschreibung (nur bei statischer IP)</h3>"));
  chunk(F("<div class='row2'>"));
  chunk(String(F("<div><label>DNS 1</label><input id='dns1' name='dns1' placeholder='leer = DHCP-DNS' value='")) + htmlEscape(cfg.dns1) + F("'></div>"));
  chunk(String(F("<div><label>DNS 2</label><input id='dns2' name='dns2' placeholder='leer = DHCP-DNS' value='")) + htmlEscape(cfg.dns2) + F("'></div>"));
  chunk(F("</div>"));
  chunk(F("<p class='hint'>Bei DHCP wird DNS-Überschreibung nicht angewendet.</p>"));
  chunk(F("</div>")); // panel wifi

  // ============================================================
  // TAB: NTP
  // ============================================================
  chunk(F("<div class='panel' id='p_ntp'>"));
  chunk(F("<h3>NTP-Server</h3>"));
  chunk(String(F("<label>NTP #1</label><input name='ntp1' value='")) + htmlEscape(cfg.ntp1) + F("'>"));
  chunk(String(F("<label>NTP #2</label><input name='ntp2' value='")) + htmlEscape(cfg.ntp2) + F("'>"));
  chunk(String(F("<label>NTP #3</label><input name='ntp3' value='")) + htmlEscape(cfg.ntp3) + F("'>"));
  chunk(String(F("<label>Synchronisationsintervall (Minuten, 0=15)</label>"
                 "<input name='ntp_interval_min' type='number' min='0' max='1440' value='")) + cfg.ntp_interval_min + F("'>"));

  if (!inAP) {
    chunk(F("<h3>NTP-Test</h3>"));
    chunk(F("<div class='row3'>"));
    chunk(F("<div><button type='button' onclick='ntpTest(1)'>▶ NTP #1 testen</button></div>"));
    chunk(F("<div><button type='button' onclick='ntpTest(2)'>▶ NTP #2 testen</button></div>"));
    chunk(F("<div><button type='button' onclick='ntpTest(3)'>▶ NTP #3 testen</button></div>"));
    chunk(F("</div>"));
    chunk(F("<div id='ntpTestResult' style='display:none;margin-top:12px'>"
            "<div class='stat'><b id='ntpTestTitle'>Test-Ergebnis</b>"
            "<div id='ntpTestStatus' style='margin:4px 0'></div>"
            "<div id='ntpTestDetail' class='mono'></div></div></div>"));
  }
  chunk(F("</div>")); // panel ntp

  // ============================================================
  // TAB: ZEITZONE
  // ============================================================
  chunk(F("<div class='panel' id='p_tz'>"));
  chunk(F("<h3>Zeitzone</h3>"));
  chunk(F("<div class='row2'>"));
  chunk(F("<div><label>UTC-Offset</label><select name='utc_offset_min'>"));
  for (int h = -12; h <= 14; h++) {
    int mins = h * 60;
    chunk(String(F("<option value='")) + mins + F("'") + selected(cfg.utc_offset_min == mins) +
          F(">UTC") + (h >= 0 ? "+" : "") + h + F("</option>"));
  }
  chunk(F("</select></div>"));
  chunk(F("<div><label>Sommer­zeitregel</label><select name='dst_mode' id='dst_mode' onchange='updDst()'>"));
  for (uint8_t m = 0; m <= 3; m++) {
    chunk(String(F("<option value='")) + m + F("'") + selected(cfg.dst_mode == m) +
          F(">") + dstModeLabel(m) + F("</option>"));
  }
  chunk(F("</select></div></div>"));
  chunk(F("<div class='row2'>"));
  chunk(String(F("<div><label>Sommerzeit fügt hinzu (Minuten)</label>"
                 "<input name='dst_add_min' type='number' min='0' max='180' value='")) + cfg.dst_add_min + F("'></div>"));
  chunk(F("<div></div></div>"));

  // Custom DST
  chunk(F("<div id='dst_custom'>"));
  chunk(F("<h3>Benutzerdefinierte Sommerzeit (M.w.d/h)</h3>"));
  chunk(F("<p class='hint'>w=1..4 (n-ter), w=5 (letzter) &nbsp; d=0 Sonntag … d=6 Samstag</p>"));
  chunk(F("<div class='row2'>"));
  chunk(String(F("<div><label>Beginn (z.B. 3.5.0/2)</label>"
                 "<input name='dst_start' value='")) +
        cfg.dst_sm + '.' + cfg.dst_sw + '.' + cfg.dst_sd + '/' + cfg.dst_sh + F("'></div>"));
  chunk(String(F("<div><label>Ende (z.B. 10.5.0/3)</label>"
                 "<input name='dst_end' value='")) +
        cfg.dst_em + '.' + cfg.dst_ew + '.' + cfg.dst_ed + '/' + cfg.dst_eh + F("'></div>"));
  chunk(F("</div></div>"));
  chunk(F("</div>")); // panel tz

  // ============================================================
  // TAB: DISPLAY
  // ============================================================
  chunk(F("<div class='panel' id='p_display'>"));
  chunk(F("<h3>Zeitformat</h3>"));
  chunk(String(F("<div class='chk'><input type='checkbox' name='show_24h' value='1'")) +
        checked(cfg.show_24h) +
        F("><label>24-Stunden-Anzeige</label></div>"));
  chunk(String(F("<div class='chk'><input type='checkbox' name='show_seconds' value='1'")) +
        checked(cfg.show_seconds) +
        F("><label>Sekunden anzeigen (nur im 24h-Modus)</label></div>"));
  chunk(F("<p class='hint'>Im 12h-Modus wird statt der Sekunden AM/PM angezeigt.</p>"));
  chunk(F("<h3>Datumsformat</h3>"));
  chunk(String(F("<select name='date_fmt'>")) +
        String(F("<option value='0'")) + selected(cfg.date_fmt==0) + F(">DD.MM.YYYY (07.03.2026)</option>") +
        String(F("<option value='1'")) + selected(cfg.date_fmt==1) + F(">MM/DD/YYYY (03/07/2026)</option>") +
        String(F("<option value='2'")) + selected(cfg.date_fmt==2) + F(">YYYY-MM-DD (2026-03-07)</option>") +
        String(F("<option value='3'")) + selected(cfg.date_fmt==3) + F(">DD.MM.YY (07.03.26)</option>") +
        String(F("<option value='4'")) + selected(cfg.date_fmt==4) + F(">MM/DD/YY (03/07/26)</option>") +
        F("</select>"));
  chunk(F("<h3>Lichtstatus</h3>"));
  chunk(String(F("<div class='stat'><b>Lux:</b> <span id='v-lux'>")) + lux_s +
        F("</span> &nbsp; <b>Sensor:</b> <span id='v-sensor'>") + sensor_s +
        F("</span> &nbsp; <b>Auto:</b> <span id='v-auto'>") + auto_s +
        F("</span> &nbsp; <b>Helligkeit:</b> <span id='v-bl'>") + bl_s +
        F("</span></div>"));
  chunk(F("<h3>Helligkeit</h3>"));
  chunk(String(F("<label>Helligkeit (0 = dunkel … 15 = maximum)</label>"
                 "<input name='brightness' type='range' min='0' max='15' value='")) +
        cfg.brightness + F("' oninput='this.nextElementSibling.value=this.value'>"
                           "<output>") + cfg.brightness + F("</output>"));
  chunk(F("<p class='hint'>Wird sofort nach dem Speichern übernommen.</p>"));
  chunk(F("<h3>Lichtsensor</h3>"));
  chunk(String(F("<div class='chk'><input type='checkbox' name='auto_brightness' value='1'")) +
        checked(cfg.auto_brightness) +
        F("><label>Helligkeit automatisch mit VEML7700 regeln</label></div>"));
  chunk(F("<p class='hint'>Wenn aktiv, folgt die Display-Helligkeit dem Umgebungslicht. Der manuelle Wert bleibt als Fallback erhalten.</p>"));
  chunk(F("<h3>Software-Dimmen (PWM)</h3>"));
  chunk(String(F("<label>Abdunklungsst&auml;rke (1=10% … 5=50% Abdunklung)</label>"
                 "<input name='pwm_duty' type='range' min='1' max='5' value='")) +
        constrain((int)cfg.pwm_duty, 1, 5) +
        F("' oninput='updPwm(this.value)'>"
          "<output id='pwm_out'>") +
        (cfg.pwm_duty < 10 ? String(cfg.pwm_duty * 10) + "%" : "Aus") +
        F("</output>"));
  chunk(String(F("<div class='chk'><input type='checkbox' name='pwm_enabled' value='1'")) +
        (cfg.pwm_duty < 10 ? " checked" : "") +
        F("><label>Software-Dimmen aktivieren</label></div>"));
  chunk(F("<p class='hint'>50 Hz PWM &uuml;ber SHUTDOWN-Register. Nur bei Hardware-Helligkeit 0 sinnvoll. "
          "Ab 50% Abdunklung kann leichtes Flackern auftreten.</p>"));
  chunk(F("<h3>Anzeigeeffekt</h3>"));
  chunk(String(F("<div class='chk'><input type='checkbox' name='scroll_enabled' value='1'")) +
        checked(cfg.scroll_enabled) +
        F("><label>Scroll-Effekt beim Ziffernwechsel</label></div>"));
  chunk(F("<p class='hint'>Ziffern der Uhrzeit scrollen von unten nach oben wenn sie sich &auml;ndern.</p>"));
  chunk(F("</div>")); // panel display

  // ============================================================
  // TAB: SYSTEM
  // ============================================================
  chunk(F("<div class='panel' id='p_system'>"));
  chunk(F("<h3>OTA-Update</h3>"));
  chunk(String(F("<div class='chk'><input type='checkbox' name='ota_enabled' value='1'")) +
        checked(cfg.ota_enabled) + F("><label style='margin:0'>OTA aktiviert</label></div>"));
  chunk(String(F("<label>OTA-Passwort (leer = kein Passwort)</label>"
                 "<input name='ota_password' type='password' value='")) + htmlEscape(cfg.ota_password) + F("'>"));
  chunk(F("<p class='hint'>OTA-Hostname: <b>dottie.local</b> — Änderungen gelten nach Neustart.</p>"));

  chunk(F("<h3>Chip-Info</h3>"));
  chunk(String(F("<div class='stat'>"
                 "<b>Chip:</b> ")) + clockChipLabel() +
        F(" &nbsp; <b>ID:</b> 0x") + clockChipIdHex() +
        F(" &nbsp; <b>Flash:</b> ") + ESP.getFlashChipSize() / 1024 + F(" KB"
        " &nbsp; <b>Freier Heap:</b> ") + ESP.getFreeHeap() + F(" Byte"
        " &nbsp; <b>Sketch-Größe:</b> ") + ESP.getSketchSize() / 1024 + F(" KB"
        " &nbsp; <b>Frei:</b> ") + ESP.getFreeSketchSpace() / 1024 + F(" KB"
        "</div>"));

  chunk(F("<h3>Aktionen</h3>"));
  chunk(F("<div class='row3'>"));
  chunk(F("<div><button formaction='/apply' formmethod='POST' class='btn-sec'>⚡ Sofort anwenden</button></div>"));
  chunk(F("<div><button formaction='/applyreboot' formmethod='POST' class='btn-sec'>⚡ Anwenden + Neustart</button></div>"));
  chunk(F("<div><button formaction='/reboot' formmethod='POST' class='btn-sec'>🔄 Neustart</button></div>"));
  chunk(F("</div>"));
  chunk(F("<div style='margin-top:10px'>"));
  chunk(F("<button formaction='/factory' formmethod='POST' class='btn-danger'"
          " onclick='return confirm(\"Wirklich Factory Reset? Alle Einstellungen werden gelöscht.\")'>"));
  chunk(F("🗑 Factory Reset</button></div>"));
  chunk(F("<h3>Diagnose</h3>"
          "<p><a href='/diag' target='_blank' style='color:var(--c)'>Diagnosedaten anzeigen</a> &mdash; zeigt WiFi-Status und gespeicherte Konfiguration.</p>"));
  chunk(F("</div>")); // panel system

  // ---- Footer: Save-Button (innerhalb des Hauptformulars!) ----
  chunk(F("<div style='position:sticky;bottom:0;background:var(--bg);padding:10px 0;"
          "text-align:right;border-top:1px solid var(--border);margin-top:16px'>"
          "<button type='submit' style='font-size:1rem;padding:10px 28px'>💾 Speichern</button>"
          "</div>"));
  chunk(F("</form>"));

  chunk(F("</div></body></html>"));
}

// ============================================================
// Web-Handler: Save
// ============================================================
static bool parseRule(const String& s, uint8_t& m, uint8_t& w, uint8_t& d, uint8_t& h) {
  int d1 = s.indexOf('.'), d2 = s.indexOf('.', d1+1), sl = s.indexOf('/', d2+1);
  if (d1<0||d2<0||sl<0) return false;
  m=(uint8_t)s.substring(0,d1).toInt(); w=(uint8_t)s.substring(d1+1,d2).toInt();
  d=(uint8_t)s.substring(d2+1,sl).toInt(); h=(uint8_t)s.substring(sl+1).toInt();
  return m>=1&&m<=12 && w>=1&&w<=5 && d<=6 && h<=23;
}

static void handleSave() {
  auto arg = [&](const char* k) -> String { return trimStr(server.arg(k)); };

  if (server.hasArg(F("sta_ssid")))   cfg.sta_ssid  = arg("sta_ssid");
  if (server.hasArg(F("sta_pass")))   cfg.sta_pass  = arg("sta_pass");

  cfg.use_dhcp = server.hasArg(F("use_dhcp"));

  if (server.hasArg(F("static_ip")))   cfg.static_ip   = arg("static_ip");
  if (server.hasArg(F("static_gw")))   cfg.static_gw   = arg("static_gw");
  if (server.hasArg(F("static_mask"))) cfg.static_mask = arg("static_mask");
  if (server.hasArg(F("dns1"))) cfg.dns1 = arg("dns1");
  if (server.hasArg(F("dns2"))) cfg.dns2 = arg("dns2");

  if (server.hasArg(F("ntp1"))) cfg.ntp1 = arg("ntp1");
  if (server.hasArg(F("ntp2"))) cfg.ntp2 = arg("ntp2");
  if (server.hasArg(F("ntp3"))) cfg.ntp3 = arg("ntp3");
  if (server.hasArg(F("ntp_interval_min"))) cfg.ntp_interval_min = (uint16_t)server.arg(F("ntp_interval_min")).toInt();

  if (server.hasArg(F("wifi_timeout_sec"))) cfg.wifi_timeout_sec = (uint16_t)server.arg(F("wifi_timeout_sec")).toInt();
  if (server.hasArg(F("ip_show_sec")))      cfg.ip_show_sec      = (uint16_t)server.arg(F("ip_show_sec")).toInt();

  if (server.hasArg(F("utc_offset_min")))   cfg.utc_offset_min   = (int16_t)server.arg(F("utc_offset_min")).toInt();
  if (server.hasArg(F("dst_mode")))         cfg.dst_mode         = (uint8_t)server.arg(F("dst_mode")).toInt();
  if (server.hasArg(F("dst_add_min")))      cfg.dst_add_min      = (int16_t)server.arg(F("dst_add_min")).toInt();

  uint8_t m,w,d,h;
  if (server.hasArg(F("dst_start")) && parseRule(arg("dst_start"),m,w,d,h))
    { cfg.dst_sm=m;cfg.dst_sw=w;cfg.dst_sd=d;cfg.dst_sh=h; }
  if (server.hasArg(F("dst_end"))   && parseRule(arg("dst_end"),m,w,d,h))
    { cfg.dst_em=m;cfg.dst_ew=w;cfg.dst_ed=d;cfg.dst_eh=h; }

  if (server.hasArg(F("brightness"))) {
    int b = constrain(server.arg(F("brightness")).toInt(), 0, 15);
    cfg.brightness = (uint8_t)b;
  }
  cfg.auto_brightness = server.hasArg(F("auto_brightness"));
  if (!server.hasArg(F("pwm_enabled"))) {
    cfg.pwm_duty = 10;
    pwmStop();
  } else if (server.hasArg(F("pwm_duty"))) {
    cfg.pwm_duty = (uint8_t)constrain(server.arg(F("pwm_duty")).toInt(), 1, 5);
    pwmStart();
  }

  cfg.show_24h     = server.hasArg(F("show_24h"));
  cfg.show_seconds = server.hasArg(F("show_seconds"));
  if (server.hasArg(F("date_fmt"))) cfg.date_fmt = (uint8_t)constrain(server.arg(F("date_fmt")).toInt(), 0, 4);
  cfg.scroll_enabled = server.hasArg(F("scroll_enabled"));
  // Scroll-State und Display-Cache zurücksetzen
  for (uint8_t _i = 0; _i < 8; _i++) scrollState[_i] = {};
  scrollInitDone = false;
  lastDispTime[0] = 0;
  lastDispDate[0] = 0;

  updateBrightnessFromLux(true);
  cfg.ota_enabled  = server.hasArg(F("ota_enabled"));
  if (server.hasArg(F("ota_password"))) cfg.ota_password = arg("ota_password");

  if (!saveConfig()) {
    server.send(500, F("text/plain"), F("Konfiguration konnte nicht gespeichert werden."));
    return;
  }

  // Im AP-Modus: Neustart damit der ESP sich mit den neuen WLAN-Daten verbindet
  if (WiFi.getMode() & WIFI_AP) {
    server.send(200, F("text/html"),
      F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
        "<meta http-equiv='refresh' content='12;url=http://192.168.4.1/'>"
        "</head><body style='font-family:sans-serif;text-align:center;padding:40px'>"
        "<h2>&#10003; Einstellungen gespeichert</h2>"
        "<p>Der ESP startet jetzt neu und verbindet sich mit dem WLAN.</p>"
        "<p>Diese Seite leitet in 12 Sekunden weiter.</p>"
        "</body></html>"));
    delay(500);
    ESP.restart();
    return;
  }

  server.sendHeader(F("Location"), F("/"));
  server.send(303);
}

// ============================================================
// Web-Handler: Apply / Reboot / Factory
// ============================================================
static void handleApply()        { applyTimeConfig(true); server.sendHeader(F("Location"),F("/")); server.send(303); }

static const char REBOOT_PAGE[] PROGMEM =
  "<!doctype html><html><head><meta charset='utf-8'>"
  "<title>Neustart...</title>"
  "<style>body{font-family:system-ui,sans-serif;display:flex;align-items:center;justify-content:center;"
  "min-height:100vh;margin:0;background:#f8fafc;color:#1e293b}"
  ".box{text-align:center;padding:40px;background:#fff;border-radius:12px;"
  "box-shadow:0 4px 24px rgba(0,0,0,.08);max-width:360px;width:90%}"
  "h2{margin:0 0 8px}.sub{color:#64748b;margin:0 0 24px}"
  ".bar{width:100%;height:6px;background:#e2e8f0;border-radius:3px;overflow:hidden;margin:20px 0}"
  ".fill{height:100%;background:#3b82f6;border-radius:3px;transition:width .4s}"
  "#msg{font-size:.9rem;color:#64748b}</style></head>"
  "<body><div class='box'>"
  "<h2>&#x1F504; Dottie startet neu</h2>"
  "<p class='sub'>Bitte warten...</p>"
  "<div class='bar'><div class='fill' id='bar' style='width:0'></div></div>"
  "<div id='msg'>Neustart wird eingeleitet...</div>"
  "</div><script>"
  "var start=Date.now(),wait=12000;"
  "function tick(){"
  "var el=Date.now()-start,pct=Math.min(100,el/wait*100);"
  "document.getElementById('bar').style.width=pct+'%';"
  "if(el<5000){document.getElementById('msg').textContent='Neustart laeuft... ('+(Math.ceil((wait-el)/1000))+'s)';"
  "setTimeout(tick,200);return;}"
  "fetch('/api/status').then(function(r){if(r.ok){window.location.href='/';}"
  "else{setTimeout(tick,800);}}).catch(function(){setTimeout(tick,800);});}"
  "setTimeout(tick,200);"
  "</script></body></html>";

static void sendRebootPage() {
  server.send_P(200, "text/html", REBOOT_PAGE);
}

static void handleApplyReboot()  { applyTimeConfig(true); sendRebootPage(); delay(400); ESP.restart(); }
static void handleReboot()       { sendRebootPage(); delay(400); ESP.restart(); }
static void handleFactory()      { server.send(200,F("text/plain"),F("Factory Reset...")); delay(200); factoryReset(); }

static void handleTest() {
  if (!server.hasArg(F("i"))) { server.send(400, F("text/plain"), F("Fehlender Parameter i")); return; }
  int i = server.arg(F("i")).toInt();
  if (i < 1 || i > 3) { server.send(400, F("text/plain"), F("i muss 1, 2 oder 3 sein")); return; }

  if (testRunning) {
    server.send(200, F("application/json"), F("{status:running}"));
    return;
  }

  pendingTestIndex = i;
  testRunning      = true;
  lastTestResult   = F("...");
  lastTestDetail   = F("Test laeuft...");
  lastTestHost     = (i==1) ? cfg.ntp1 : (i==2) ? cfg.ntp2 : cfg.ntp3;
  lastTestMs       = millis();

  // Sofort antworten — der eigentliche Test läuft in loop()
  server.send(200, F("application/json"), F("{status:started}"));
}

// Wird aus loop() aufgerufen — außerhalb des Web-Handlers, DNS funktioniert hier
static void runPendingNtpTest() {
  if (pendingTestIndex < 0 || !testRunning) return;

  int i = pendingTestIndex;
  pendingTestIndex = -1;   // sofort zurücksetzen, damit nicht doppelt läuft

  String host = (i==1) ? cfg.ntp1 : (i==2) ? cfg.ntp2 : cfg.ntp3;

  // Schritt 1: DNS — hier in loop() funktioniert hostByName zuverlässig
  IPAddress resolvedIp;
  String resolveLog;
  bool dnsOk = false;

  // DNS: mehrere Versuche mit yield() dazwischen — hostByName braucht
  // manchmal mehrere loop()-Iterationen bis der lwIP-Stack antwortet
  for (uint8_t attempt = 0; attempt < 8 && !dnsOk; attempt++) {
    // Zwischen Versuchen loop()-Zeit lassen damit lwIP arbeiten kann
    for (uint8_t w = 0; w < 20; w++) { delay(10); yield(); }
    if (WiFi.hostByName(host.c_str(), resolvedIp)) {
      dnsOk = true;
      resolveLog = String(F("hostByName:OK ip=")) + resolvedIp.toString()
                 + F(" (Versuch ") + (attempt+1) + F(")");
      cachePut(host, resolvedIp);
    }
  }

  if (!dnsOk) {
    // Fallback 1: lwIP direkt
    IPAddress lwIp; String lwDbg;
    if (dnsResolveA_lwip(host, lwIp, lwDbg, 4000)) {
      resolvedIp = lwIp; dnsOk = true;
      resolveLog = String(F("lwip:OK ip=")) + lwIp.toString();
      cachePut(host, resolvedIp);
    } else {
      resolveLog = String(F("hostByName:FAIL | ")) + lwDbg;
      // Fallback 2: Cache
      uint32_t ageSec;
      if (cacheGet(host, resolvedIp, ageSec)) {
        dnsOk = true;
        resolveLog += String(F(" | CACHE:OK ip=")) + resolvedIp.toString()
                    + F(" age=") + ageSec + "s";
      }
    }
  }

  resolveLog += String(F("  [dns0=")) + ipOrLabel(WiFi.dnsIP(0)) + F("]");

  // Schritt 2: NTP UDP-Probe
  String probeDetail;
  bool ok = false;
  uint32_t txSec = 0;
  if (dnsOk) {
    ok = ntpProbeByIp(resolvedIp, NTP_PROBE_TIMEOUT_MS * 2, &probeDetail, &txSec);
  } else {
    probeDetail = F("DNS fehlgeschlagen");
  }

  if (ok && txSec) syncSystemTimeFromNtp(txSec);

  lastTestHost   = host;
  lastTestResult = ok ? F("OK") : F("FAIL");
  lastTestDetail = resolveLog + F("\nUDP: ") + probeDetail;
  lastTestMs     = millis();
  testRunning    = false;

  ntpLogAdd(time(nullptr), host, ok, lastTestDetail);
}

// Dummy — wird nicht mehr aufgerufen, aber der Compiler braucht die Signatur

// ============================================================
// Web-Setup
// ============================================================
static void handleApiNtpTest() {
  // Build JSON manually - avoid String escape issues
  String json = "{\"running\":";
  json += testRunning ? "true" : "false";
  json += ",\"host\":\"";
  json += jsonEscape(lastTestHost);
  json += "\",\"result\":\"";
  json += jsonEscape(lastTestResult);
  json += "\",\"detail\":\"";
  json += jsonEscape(lastTestDetail);
  json += "\"}";
  server.sendHeader(F("Cache-Control"), F("no-cache"));
  server.send(200, F("application/json"), json);
}

static void handleApiStatus() {
  time_t now = time(nullptr);
  char tbuf[20] = "--:--:--";
  char dbuf[14] = "--.--.----";
  struct tm tm_l; memset(&tm_l, 0, sizeof(tm_l));
  bool timeValid = (now > 1700000000UL);
  if (timeValid) {
    localtime_r(&now, &tm_l);
    snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", tm_l.tm_hour, tm_l.tm_min, tm_l.tm_sec);
    formatDateDmy(dbuf,
                  (uint8_t)tm_l.tm_mday,
                  (uint8_t)(tm_l.tm_mon + 1),
                  (uint16_t)(tm_l.tm_year + 1900));
  }
  // Uptime
  uint32_t s=(millis()-bootMs)/1000; uint32_t m=s/60; s%=60;
  uint32_t h=m/60; m%=60; uint32_t d=h/24; h%=24;
  char upbuf[24]; snprintf(upbuf, sizeof(upbuf), "%lud %02lu:%02lu:%02lu",
                           (unsigned long)d,
                           (unsigned long)h,
                           (unsigned long)m,
                           (unsigned long)s);

  String json = "{";
  json += String(F("\"time\":\"")) + tbuf + F("\",");
  json += String(F("\"date\":\"")) + dbuf + F("\",");
  json += String(F("\"uptime\":\"")) + upbuf + F("\",");
  json += String(F("\"epoch\":")) + (unsigned long)now + F(",");
  // localsec: Sekunden seit Tagesbeginn in Lokalzeit — für JS-Interpolation
  json += String(F("\"localsec\":")) +
          (timeValid ? (uint32_t)(tm_l.tm_hour * 3600 + tm_l.tm_min * 60 + tm_l.tm_sec) : 0UL)
          + F(",");
  json += String(F("\"lux\":")) + (isfinite(luxHold) ? String(luxHold, 1) : String(-1.0f)) + F(",");
  json += String(F("\"brightness\":")) + displayIntensityApplied + F(",");
  json += String(F("\"auto_brightness\":")) + (cfg.auto_brightness ? F("true") : F("false")) + F(",");
  json += String(F("\"sensor_ok\":")) + (vemlOk ? F("true") : F("false")) + F(",");
  json += String(F("\"sensor_msg\":\"")) + jsonEscape(vemlDiag) + F("\",");
  json += String(F("\"nextsync\":")) + nextSyncInSec() + F(",");
  json += String(F("\"heap\":")) + ESP.getFreeHeap() + F(",");
  json += String(F("\"rssi\":")) + (WiFi.status()==WL_CONNECTED ? WiFi.RSSI() : 0);
  json += "}";

  server.sendHeader(F("Cache-Control"), F("no-cache"));
  server.send(200, F("application/json"), json);
}

static void handleDiag() {
  String out;
  out += F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
           "<style>body{font-family:monospace;padding:20px}pre{background:#111;color:#0f0;padding:15px;border-radius:6px}</style>"
           "</head><body><h2>Dottie Diagnose</h2><pre>");
  out += "FW:       "; out += FPSTR(FW_VERSION);       out += "\n";
  out += "SSID:     ["; out += htmlEscape(cfg.sta_ssid); out += "]\n";
  out += "Passwort: ";
  if (cfg.sta_pass.length()) { out += "(gesetzt, "; out += cfg.sta_pass.length(); out += " Zeichen)"; }
  else out += "(LEER - kein Passwort gespeichert!)";
  out += "\n";
  out += "DHCP:     "; out += (cfg.use_dhcp ? "ja" : "nein"); out += "\n";
  out += "Stat.IP:  "; out += htmlEscape(cfg.static_ip); out += "\n";
  out += "WiFi:     ";
  if (WiFi.status() == WL_CONNECTED) { out += WiFi.localIP().toString(); out += " (verbunden)"; }
  else { out += "NICHT verbunden (status="; out += (int)WiFi.status(); out += ")"; }
  out += "\n";
  out += "AP-Modus: "; out += ((WiFi.getMode()&WIFI_AP) ? "ja" : "nein"); out += "\n";
  out += "DNS0:     "; out += WiFi.dnsIP(0).toString(); out += "\n";
  out += "Heap:     "; out += ESP.getFreeHeap(); out += "\n";
  out += "NTP1:     "; out += htmlEscape(cfg.ntp1); out += "\n";
  out += F("</pre><p><a href='/'>Zuruck zur Hauptseite</a></p></body></html>");
  server.send(200, F("text/html"), out);
}

static void setupWeb() {
  server.on(F("/"),            handleRoot);
  server.on(F("/save"),        HTTP_POST, handleSave);
  server.on(F("/apply"),       HTTP_POST, handleApply);
  server.on(F("/applyreboot"), HTTP_POST, handleApplyReboot);
  server.on(F("/reboot"),      HTTP_POST, handleReboot);
  server.on(F("/factory"),     HTTP_POST, handleFactory);
  server.on(F("/test"),        HTTP_POST, handleTest);
  server.on(F("/test"),        HTTP_GET,  handleTest);
  server.on(F("/api/status"),  HTTP_GET,  handleApiStatus);
  server.on(F("/api/ntptest"), HTTP_GET,  handleApiNtpTest);
  server.on(F("/diag"),        HTTP_GET,  handleDiag);
  server.begin();
}
