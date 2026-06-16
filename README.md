# qs1200-esphome

ESPHome external component voor de **Intex QS1200** zoutwatersysteem  
(ook bekend als **ECO 6220G / CG-26670**).

De ESP32 functioneert als **man-in-the-middle** tussen het Intex-hoofdbord en het display/keypad-board.  
De originele veiligheidslogica van Intex (flowbeveiliging, zoutmeting, celsturing, foutdetectie) blijft volledig intact.

---

## Architectuur

```
Intex mainboard  ←──DIO──→  ESP32  ←──DIO──→  display/keypad board
                 ←──CLK──→         ←──CLK──→
```

De ESP32:
- **sniift** de DIO-lijn (mainboard → display) en decodeert display- en statusdata
- **stuurt door** alle signalen transparant in beide richtingen (fail-safe pass-through)
- **injecteert** (fase 2) knopcommando's richting het hoofdbord

Bij een crash of reset van de ESP32 vallen de uitgangslijnen terug op HIGH (idle), waardoor het originele systeem normaal blijft functioneren.

---

## Gefaseerde roadmap

| Fase | Status | Inhoud |
|------|--------|--------|
| 1 | ✅ Gereed | Pass-through + display/status uitlezen → HA sensoren |
| 2 | 🔲 Gepland | Knopinjectie (power, timer, boost, self-clean, lock) |
| 3 | 🔲 Gepland | Runtime-abstractie (looptijd in uren, zelfreinigingsinterval) |

---

## Home Assistant entiteiten

### Fase 1 — sensoren

| Entiteit | Type | Omschrijving |
|----------|------|--------------|
| `sensor.qs1200_display_code` | text_sensor | Displaywaarde, bijv. `00`, `80`, `90`–`93` |
| `binary_sensor.qs1200_running` | binary_sensor | Eenheid actief (chloreert) |
| `binary_sensor.qs1200_boost` | binary_sensor | Boost-modus actief |
| `binary_sensor.qs1200_sleep` | binary_sensor | Slaapstand actief |
| `binary_sensor.qs1200_low_flow` | binary_sensor | Lage doorstroming (pomp) |
| `binary_sensor.qs1200_low_salt` | binary_sensor | Zoutgehalte te laag |
| `binary_sensor.qs1200_high_salt` | binary_sensor | Zoutgehalte te hoog |
| `binary_sensor.qs1200_service` | binary_sensor | Servicefout actief |

### Fase 2 — knoppen

| Entiteit | Omschrijving |
|----------|--------------|
| `button.qs1200_power` | Aan/Uit |
| `button.qs1200_timer` | Timer (één druk = +1 uur) |
| `button.qs1200_boost` | Boost starten/stoppen |
| `button.qs1200_self_clean` | Zelfreinigen starten |
| `button.qs1200_lock` | Vergrendeling aan/uit |

### Fase 3 — abstracties

| Entiteit | Omschrijving |
|----------|--------------|
| `number.qs1200_runtime_hours` | Looptijd instellen (1–12 uur) |
| `select.qs1200_self_clean_interval` | Zelfreinigingsinterval (6h / 10h / 14h) |

---

## Protocol

Het non-TM1650 protocol is gedocumenteerd in [jingsno/intex-swg-pcb](https://github.com/jingsno/intex-swg-pcb/tree/main/protocol).

### DIO-lijn (mainboard → display) — 32-bit frame

Pulsbreedte-modulatie, LSB-first transmissie:

| Puls | Tijdsduur |
|------|-----------|
| Start (LOW) | 200 µs |
| Bit 1: HIGH | 800 µs |
| Bit 0: HIGH | 200 µs |
| Spacer (LOW) | 200 µs |

**Frame-indeling (na decode, MSB links):**

```
bit 31–24   bit 23–16   bit 15–0
[linker 7-seg] [rechter 7-seg] [LED-statusbits]
```

**LED-statusbits:**

| Bit | Betekenis |
|-----|-----------|
| 15 | Boost |
| 12 | Slaapstand |
| 11 | Actief / Working |
| 10 | Ozone |
| 6 | Lage doorstroming |
| 5 | Zout te laag |
| 1 | Zout te hoog |
| 0 | Servicefout |

### CLK-lijn (display → mainboard) — 16-bit knopframe

```
byte 1 = ~byte 0   byte 0 = knopcode
```

| Knop | Code |
|------|------|
| Power | `0x04` |
| Timer | `0x02` |
| Boost | `0x10` |
| Self-clean | `0x08` |
| Lock | `0x01` |
| Release | `0x00` |

---

## Hardware

### Benodigdheden

- ESP32 development board (bijv. ESP32-DevKitC)
- Bidirectionele level shifter (bijv. **TXS0108E**) — meet de signaalspanning eerst na (verwacht 5V)
- Buck converter 12V → 5V voor de ESP32-voeding (of gebruik de +5V van het Intex-bord indien beschikbaar en stabiel genoeg)
- JST/Dupont bedrading

### Pin-toewijzing (standaard)

| ESP32 GPIO | Richting | Verbinding |
|-----------|----------|------------|
| GPIO18 | ← input | DIO van mainboard (FROM_MAIN) |
| GPIO16 | → output | DIO naar displayboard (TO_DISP) |
| GPIO17 | ← input | CLK van displayboard (FROM_DISP) |
| GPIO19 | → output | CLK naar mainboard (TO_MAIN) |
| GND | — | DC logic GND van Intex-bord |

> **Waarschuwing:** verbind GND met de DC logic GND, niet met de AC-zijde of willekeurig aan het chassis.

### Level shifting

Meet de signaalspanningen op DIO en CLK met een multimeter (DC, unit uitgeschakeld en ingeschakeld).  
Als de spanningen > 3,3V zijn: gebruik een level shifter tussen het Intex-bord en de ESP32-ingangen.  
De ESP32-ingangen zijn **niet** 5V-tolerant.

### PCB-referentie

De PCB-layout en aansluitpunten voor de ECO6220G zijn te vinden in [cspiel1/pool](https://github.com/cspiel1/pool) (foto `img.jpeg`).

---

## Installatie

### 1. Repository klonen

```bash
git clone https://github.com/lt-goldman/qs1200-esphome.git
cd qs1200-esphome
```

### 2. `secrets.yaml` invullen

```yaml
wifi_ssid: "jouw-ssid"
wifi_password: "jouw-wifi-wachtwoord"
fallback_password: "qs1200-fallback"
ha_api_key: "base64-sleutel-uit-esphome"
ota_password: "jouw-ota-wachtwoord"
```

### 3. Pinnen controleren in `qs1200.yaml`

Pas de GPIO-nummers aan als je andere pinnen gebruikt.

### 4. Flashen

```bash
esphome run qs1200.yaml
```

### 5. Logs controleren

Zet `logger: level: VERBOSE` om elke ontvangen frame te zien:

```
[V][qs1200:123]: frame=0x3F3F0800 display='00' W=1 B=0 S=0 LF=0 LS=0 HS=0 SVC=0
```

---

## Fase 2 activeren

Uncomment de `button:` sectie in `qs1200.yaml`.  
Test eerst met fase 1 (read-only) totdat de frames stabiel binnenkomen.

---

## Veiligheidsaanwijzingen

- Schakel het Intex-systeem uit en haal de stekker eruit voordat je bedrading aansluit.
- De ESP32 stuurt **nooit** rechtstreeks de chloorcel of het relaisboard aan.
- De originele Intex-controller blijft verantwoordelijk voor alle veiligheidslogica.
- Bij een reset van de ESP32 blijven de pass-through lijnen in idle-toestand (HIGH), zodat het originele systeem ongestoord verder werkt.

---

## Versiegeschiedenis

| Versie | Omschrijving |
|--------|--------------|
| v0.1.6 | Bugfix: FROM_DISP->TO_MAIN inversie gecorrigeerd (directe pass-through) |
| v0.1.5 | Versiehistorie bijgewerkt |
| v0.1.4 | Plain ASCII formatting, includes met dubbele quotes |
| v0.1.3 | FreeRTOS includes, button.py en text_sensor.py opgeschoond |
| v0.1.2 | Compile-fouten opgelost, gevalideerd met ESPHome 2026.5.3 |
| v0.1.1 | README toegevoegd |
| v0.1.0 | Initiële versie: fase 1 read-only component |

---

## Licentie

MIT
