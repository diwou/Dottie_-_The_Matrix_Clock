# Dottie – The Matrix Clock

Eine NTP-synchronisierte Dot-Matrix-Uhr auf Basis eines **ESP8266 (NodeMCU v2)** mit 16 MAX7219-LED-Matrix-Modulen. Zeigt Zeit und Datum mit Scroll-Animation, Weboberfläche, OTA-Updates und automatischer Zeitsynchronisation.

- Aktuelle Version: `1.4.7`
- Repository: `https://codeberg.org/diwou/Dottie_-_The_Matrix_Clock`
- Lizenz: `CC BY-NC-SA 4.0`

---

## Features

- **NTP-Synchronisation** mit 3 konfigurierbaren Fallback-Servern
- **Zeitzone & DST**: UTC-Offset, EU/US/Custom-Sommerzeit
- **Display**: 24h/12h-Modus, 5 Datumsformate, einstellbare Helligkeit, Scroll-Animation
- **Web-UI**: Responsive HTML5-Oberfläche mit Live-Display-Vorschau
- **OTA-Updates**: Kabellose Firmware-Updates über `dottie.local`
- **WiFi-Resilienz**: State-Machine mit exponentiellem Backoff, AP-Fallback (Captive Portal)
- **Factory-Reset**: GPIO16 5 Sekunden halten

---

## Hardware

| Komponente | Details |
|---|---|
| MCU | ESP8266 NodeMCU v2 |
| Display | 16× MAX7219 LED-Matrix-Module (FC16_HW) |
| SPI | CS = D2 (GPIO4) |
| Anordnung | 2 Reihen: TOP (8 Module, 8×8 px) + BOTTOM (8 Module, 5×7 px) |
| Reset-Button | GPIO16 (D0) – externer 10 kΩ Pull-Up zu 3,3 V erforderlich! |

### Verdrahtung

```
NodeMCU → MAX7219-Kette
D7 (MOSI/GPIO13) → DIN
D5 (CLK/GPIO14)  → CLK
D2 (CS/GPIO4)    → CS/LOAD
```

Die 16 Module bilden eine SPI-Kette: TOP (Module 0–7, rechts nach links) und BOTTOM (Module 8–15, rechts nach links). Das physikalische Mapping wird über `MAP_TOP[]` und `MAP_BOTTOM[]` in `display.h` konfiguriert.

---

## Software / Build

**Werkzeug:** PlatformIO (VSCode/VSCodium Extension oder CLI)

```bash
# Kompilieren
pio run -e nodemcuv2

# OTA-Upload (IP in platformio.ini anpassen!)
pio run -t upload -e nodemcuv2

# Serieller Monitor (USB-Fallback, 115200 Baud)
pio device monitor
```

### Abhängigkeiten

| Bibliothek | Version |
|---|---|
| `majicdesigns/MD_MAX72XX` | ^3.5.1 |
| ESP8266 Arduino Core (Board Package) | – |

---

## Inbetriebnahme

1. `platformio.ini` öffnen und `upload_port` auf die IP-Adresse des Geräts setzen (oder Serial-Upload verwenden)
2. Firmware flashen: `pio run -t upload -e nodemcuv2`
3. Beim ersten Start ohne gespeicherte WLAN-Konfiguration öffnet Dottie einen **AP** mit SSID `DOTTIE-{ChipID}`
4. Mit dem AP verbinden (IP: `192.168.4.1`) und WLAN-Zugangsdaten eingeben
5. Nach dem Verbinden ist die Web-UI unter der zugewiesenen IP oder `http://dottie.local` erreichbar

---

## Web-Oberfläche

### Tabs

| Tab | Inhalt |
|---|---|
| **Status** | WiFi-Status, IP, NTP-Log (letzte 5 Syncs), manueller NTP-Test |
| **WiFi** | SSID, Passwort, Timeout, DHCP/Static-IP, DNS |
| **NTP** | 3 Server-URLs, Sync-Intervall, Test-Buttons |
| **Zeitzone** | UTC-Offset, DST-Modus (Aus/EU/US/Custom) |
| **Display** | 24h/12h, Sekunden, Datumsformat, Helligkeit, Scroll-Effekt |
| **System** | OTA, Chip-Info, Neustart, Factory-Reset, Diagnose |

### REST-API

| Endpoint | Methode | Beschreibung |
|---|---|---|
| `/api/status` | GET | JSON: Zeit, Datum, Uptime, next_sync, Heap, RSSI |
| `/api/ntptest` | GET | JSON: running, host, result, detail |
| `/save` | POST | Konfiguration speichern |
| `/test?i=1..3` | POST | NTP-Server testen (asynchron) |
| `/reboot` | POST | Neustart |
| `/factory` | POST | Factory-Reset |
| `/diag` | GET | Diagnose-Seite (Debug-Info) |

---

## Konfiguration

Die Konfiguration wird als `Key=Value`-Datei auf LittleFS unter `/cfg.txt` gespeichert. Kommentare mit `#` sind erlaubt.

### Wichtige Parameter

| Schlüssel | Standardwert | Beschreibung |
|---|---|---|
| `ntp1..3` | pool.ntp.org, time.google.com, time.cloudflare.com | NTP-Server |
| `ntp_interval_min` | 15 | Sync-Intervall in Minuten (0 = 15) |
| `utc_offset_min` | 60 | UTC-Offset in Minuten (UTC+1 = 60) |
| `dst_mode` | 1 | 0=Aus, 1=EU, 2=US, 3=Custom |
| `date_fmt` | 0 | Datumsformat (0–4, s.u.) |
| `show_24h` | 1 | 24h-Modus (0 = 12h mit AM/PM) |
| `show_seconds` | 1 | Sekunden anzeigen (nur 24h) |
| `brightness` | 1 | Display-Helligkeit (0–15) |
| `scroll_enabled` | 1 | Scroll-Animation beim Ziffernwechsel |

### Datumsformate

| Wert | Format | Beispiel |
|---|---|---|
| 0 | `DD.MM.YYYY` | 07.03.2026 |
| 1 | `MM/DD/YYYY` | 03/07/2026 |
| 2 | `YYYY-MM-DD` | 2026-03-07 |
| 3 | `DD.MM.YY` | 07.03.26 |
| 4 | `MM/DD/YY` | 03/07/26 |

---

## Architektur

### Dateistruktur

| Datei | Rolle |
|---|---|
| `src/main.cpp` | Kernlogik: Boot, WiFi-State-Machine, NTP-Sync, OTA, Loop |
| `src/config.h` | Config-Struct, LittleFS-Load/Save (`/cfg.txt`), Makro-Parser |
| `src/display.h` | Bitmap-Glyphen (8×8 + 5×7), Scroll-Animation, Display-Mapping |
| `src/webserver.h` | HTML5-WebUI (Tabs), REST-APIs, JavaScript-Glyphen-Renderer |
| `platformio.ini` | Build-Konfiguration, Bibliotheksabhängigkeiten |

### WiFi-State-Machine

```
CONNECTED
  ↓ (Verbindungsverlust)
DISCONNECTED
  ↓ (nach Backoff: 30s → 60s → 120s → 300s max)
RECONNECTING
  ↓ (max. Fehlversuche erreicht)
AP_MODE (Captive Portal: SSID DOTTIE-{ChipID}, IP 192.168.4.1)
```

### NTP-Synchronisation

1. DNS-Auflösung (3 Versuche, lwIP-Direktfallback)
2. IP-Cache (3 Einträge) für schnelle Re-Syncs
3. UDP-Probe nach RFC 5905, Timeout 900 ms
4. 3 Server werden getestet, erster erfolgreicher wird verwendet
5. Log-Ringpuffer (5 Einträge) in der Web-UI sichtbar

### Display-Mapping

Physikalische Modul-IDs werden über `MAP_TOP[8]` und `MAP_BOTTOM[8]` von logischen Positionen entkoppelt. Flip/Mirror/Rotate für das Bottom-Display sind über Defines konfigurierbar (`BOTTOM_FLIP_VERTICAL_8`).

### OTA

- Hostname: `dottie.local` (mDNS)
- Progress-Anzeige auf dem Matrix-Display (Prozentbalken)
- OTA-Passwort optional konfigurierbar

---

## Factory-Reset

GPIO16 (D0) für **5 Sekunden** gedrückt halten:
- Countdown wird auf dem Display angezeigt
- `/cfg.txt` wird gelöscht
- Gerät startet neu und öffnet AP

---

## Lizenz

Dieses Projekt steht unter der Lizenz `Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)`.

Lizenztext: `LICENSE`
