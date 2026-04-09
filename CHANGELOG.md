# Changelog – Dottie – The Matrix Clock

Alle nennenswerten Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

Format orientiert sich an [Keep a Changelog](https://keepachangelog.com/de/1.0.0/).

---

## [1.4.7] – 2026-04-09

### Geändert
- Projekt für die Veröffentlichung auf Codeberg vorbereitet: `README.md` auf Versionsstand, Repository-URL und Lizenzangaben aktualisiert.
- `.gitignore` für ein sauberes öffentliches Repository geschärft, damit lokale KI-/IDE-Dateien, Build-Artefakte und temporäre Dateien nicht mit ins Git gelangen.
- Firmware-Versionsstring auf `1.4.7` angehoben, damit Code, Dokumentation und Release-Stand konsistent sind.

---

## [1.4.6] – 2026-03-28

### Verbessert
- **Subsekunden-Kompensation für synchrone Scroll-Animationen:** Zwei Uhren am gleichen NTP-Server animieren den Sekundenwechsel jetzt auf ≤ 1 Frame (~20ms) genau synchron.
  - `drawTimeHHMMSS()`: beim Scroll-Start wird per `gettimeofday()` die aktuelle Position innerhalb der Sekunde ermittelt; bereits vergangene Animation-Steps werden übersprungen, sodass die Uhr sofort im richtigen Frame einsteigt.
  - Scroll-Tick-Timer: `lastScrollMs += 20` statt `= millis()` verhindert akkumulierten Drift. Beim ersten Tick wird der Timer auf das Sub-Sekunden-20ms-Raster ausgerichtet.
  - Display-Update-Intervall von 200 ms auf 50 ms reduziert: Sekundenwechsel werden bis zu 4× früher erkannt (Erkennungsjitter ±50 ms statt ±200 ms).

---

## [1.4.5] – 2026-03-19

### Behoben
- `LOST`-Anzeige blieb dauerhaft im unteren Display stehen, wenn die WLAN-Verbindung kurz unterbrochen und sofort wiederhergestellt wurde. Ursache: `lastDispDate` wurde nach dem `drawBottomText5x7Centered("LOST")`-Aufruf nicht invalidiert, sodass der Loop-Vergleich keinen Unterschied erkannte und das Display nicht neu zeichnete. Fix: `lastDispTime` und `lastDispDate` werden nach der LOST-Anzeige geleert, um einen erzwungenen Redraw auszulösen.

---

## [1.4.4]

### Geändert
- PWM auf `Ticker`-Bibliothek umgestellt: Die Phasenwechsel werden jetzt unabhängig vom Haupt-Loop über `yield()`-Callbacks getriggert. Der Ticker feuert auch dann, wenn der Loop durch `server.handleClient()`, `ArduinoOTA.handle()` oder `MDNS.update()` belegt ist – Loop-Jitter hat damit keinen Einfluss mehr auf die PWM-Zeiten

### Behoben
- OTA: `pwmStop()` wird jetzt im `onStart`-Callback aufgerufen, stoppt den Ticker und schaltet das Display sicher ein

---

## [1.4.3]

### Behoben
- OTA: Display war während des Uploads dunkel, weil `ArduinoOTA.handle()` den Loop blockiert und die PWM-Dunkelphase nicht beendet werden konnte. Im `onStart`-Callback wird SHUTDOWN nun explizit aufgehoben
- PWM-Periode von 40 ms (25 Hz) auf 20 ms (50 Hz) korrigiert: 25 Hz lag im klar sichtbaren Flicker-Bereich und verursachte starkes Flimmern nach OTA-Neustart
- PWM-State-Machine: `phaseStartMs` wird beim ersten Aufruf auf `millis()` initialisiert, verhindert sofortigen Dark-Burst beim Systemstart

---

## [1.4.2]

### Behoben
- PWM-Pulsrichtung korrigiert: kurze Pulse liegen jetzt auf der Dunkelseite, nicht der Hellseite → keine hellen Blitze mehr bei niedrigen Abdunklungsstufen; robuster gegenüber ungleichmäßigen Loop-Durchlaufzeiten
- `g_pwmOn`-Initialzustand korrigiert (war `true` statt `false`) → Restore auf „Kein PWM" schaltet Display nun zuverlässig wieder ein
- Label „Einschaltzeit" → „Abdunklungsstärke" (entspricht jetzt dem tatsächlichen Effekt: 10% = 10% der Zeit dunkel)

---

## [1.4.0]

### Neu
- **Software-PWM-Dimmer**: Helligkeit unterhalb der Hardware-Stufe 0 durch 50-Hz-SHUTDOWN-PWM des MAX7219
  - Schieberegler im Display-Tab (1–5, entspricht 10–50 % Einschaltzeit) + Checkbox zum Aktivieren
  - Nützlicher Bereich: 10–50 % (ab ~60 % wird Flackern wahrnehmbar)
  - Neue Config-Option `pwm_duty` (Standard: 10 = deaktiviert)
- **Neustart-Seite**: Nach Reboot aus dem Webinterface wartet der Browser automatisch auf den ESP und lädt die Startseite neu

### Behoben
- SHUTDOWN-Register-Logik korrigiert (ON/OFF waren invertiert → Display blieb nach Deaktivieren des PWM dauerhaft dunkel)

---

## [1.3.1]

### Neu
- Umbenennung: "NTP Uhr" → **"Dottie – The Matrix Clock"**
- mDNS/OTA-Hostname geändert: `ntpclock` → `dottie` (`dottie.local`)
- AP-SSID-Prefix geändert: `NTP-UHR-` → `DOTTIE-`

### Behoben
- `GLYPH_M` (8×8) korrigiert: 2-Pixel-breite Striche passend zum restlichen Font

---

## [1.3.0]

### Neu
- **12h-Modus** mit AM/PM-Anzeige (im 12h-Modus entfallen die Sekunden, stattdessen AM/PM)
- Sekunden im 24h-Modus nun **deaktivierbar** (Display-Tab)
- **5 Datumsformate** wählbar:
  - `DD.MM.YYYY` (Standard)
  - `MM/DD/YYYY`
  - `YYYY-MM-DD` (ISO 8601)
  - `DD.MM.YY`
  - `MM/DD/YY`
- Neue Glyphen: `M` (8×8), `/` und `-` (5×7) für die neuen Formate

---

## [1.2.5]

### Behoben
- Fehlende Glyphen ergänzt:
  - 8×8: `%`, `?`
  - 5×7: `A`, `B`, `H`, `L`

---

## [1.1.0] – Major Refactor

### Neu
- **OTA (ArduinoOTA)**: Kabellose Firmware-Updates, Progress-Anzeige auf dem Matrix-Display
- **NTP-Sync-Protokoll**: Ringpuffer im RAM (letzte 5 Einträge), in Web-UI sichtbar
- **Automatischer WiFi-Reconnect** mit State-Machine und exponentiellem Backoff (30 s → 300 s)
- **Uptime-Anzeige** in der Web-UI
- **mDNS**: Gerät unter `dottie.local` erreichbar

### Verbessert
- **Web-UI** komplett neu gestaltet: modernes Design, Tabs (Status/WiFi/NTP/Zeitzone/Display/System), responsive Layout, Live-Display-Vorschau (Canvas)
- **Speicher**: PROGMEM für HTML-Strings, `F()`-Makros konsequent überall
- **Stabilität**: vollständige WiFi-Reconnect-Logik, AP-Fallback (Captive Portal)
- **NTP-Probe**: robustere Implementierung mit DNS-Fallback, IP-Cache, Fehlerklassifizierung
- **Code-Qualität**: Konstanten statt Magic-Numbers, klare Benennung, kein duplizierter Code

---

## [1.0.x] – Initiale Versionen

Grundlegende Funktionalität:
- NTP-Synchronisation (einzelner Server)
- Zeitanzeige auf TOP-Matrix (8×8-Glyphen)
- Datumsanzeige auf BOTTOM-Matrix (5×7-Glyphen)
- Einfache Web-UI zur WLAN-Konfiguration
- LittleFS-Konfigurationsdatei (`/cfg.txt`)
- Factory-Reset via GPIO16
