# USB-C Power Meter - Verbesserungsvorschläge für v2.4

## 🎯 IHRE ERWÄHNTEN PROBLEME

### 1. **TPS2115 Umschaltverhalten** ✓ Ihre Lösung ist korrekt

```
Problem: Spannungseinbruch beim Umschalten zwischen VBUS und BATT
Lösung: C11: 10µF → 47µF Tantal

✅ SEHR GUT! Tantalum hat:
- Niedrigere ESR als Keramik (typisch 100-500mΩ vs 5-50mΩ)
- Bessere Ladungsspeicherung
- Geringerer Spannungseinbruch

Empfehlung:
- AVX TAJB476K016R (47µF, 16V, ESR<1Ω, Case B)
- Kemet T491B476K016AT (47µF, 16V, ESR<2Ω)
- PARALLEL: 100nF X7R Keramik (für HF-Entkopplung)

Alternative (falls kein Tantal):
- 2x 22µF Keramik parallel (low ESR, X7R/X5R)
- Murata GRM32ER71C226KE20L (22µF, 16V, X7R, 1210)
```

---

### 2. **INA228 Offset-Problem: 4-8mA im Leerlauf** 🔴 KRITISCH

**Ursachen-Analyse:**

#### **A) Thermische Effekte (temperaturabhängig!)**

Die Temperaturabhängigkeit deutet auf:

**1. Shunt-Widerstand Tempco (Temperature Coefficient)**

```
Problem: 
- Standard-Widerstände haben TCR (Temperature Coefficient of Resistance)
- Typisch: 50-200 ppm/°C
- Bei Messung von mA-Bereich: Massiver Einfluss!

Beispiel:
- Shunt: 10mΩ, TCR=100ppm/°C
- ΔT = 40°C (Raumtemp → Betriebstemp)
- ΔR = 10mΩ × 100ppm/°C × 40°C = 40µΩ
- Bei 1A Strom: ΔV = 40µV → Fehler: 4mA!

Lösung: NIEDRIG-TCR SHUNT verwenden!
```

**Empfohlene Shunt-Widerstände:**

| Typ                     | TCR        | Preis | Anwendung        |
|-------------------------|------------|-------|------------------|
| **Vishay WSL2512**      | ±50 ppm/°C | €     | Budget           |
| **Isabellenhuette PBV** | ±20 ppm/°C | €€    | Empfohlen        |
| **Vishay WSLP3921**     | ±5 ppm/°C  | €€€   | Präzision        |
| **KOA PSR**             | ±15 ppm/°C | €€    | Guter Kompromiss |

```
Konkrete Empfehlung für v2.4:
- Isabellenhuette PBV-R010-1.0 (10mΩ, ±20ppm/°C, 2512, 1W)
- Vishay WSLP2512R0100FEA (10mΩ, ±1%, ±5ppm/°C, Premium)
- KOA PSR400-10-R010J (10mΩ, ±5%, ±15ppm/°C, Mittelklasse)
```

**2. INA228 Offset-Drift**

```
INA228 Specs (aus Datenblatt):
- Offset Voltage: ±10µV (typ), ±50µV (max)
- Offset Drift: ±0.5µV/°C (typ), ±2µV/°C (max)

Bei 40°C Temperaturanstieg:
- Offset-Änderung: 0.5-2µV/°C × 40°C = 20-80µV
- Bei 10mΩ Shunt: 2-8mA Fehler! ← PASST ZU IHRER BEOBACHTUNG!

Lösungen:
a) Offset-Kalibrierung implementieren (Software)
b) Bessere thermische Kopplung: INA228 nah am Shunt
c) Kelvin-Verbindung optimieren (siehe unten)
```

#### **B) Layout-Probleme (häufigste Ursache!)**

**1. Kelvin-Verbindung nicht korrekt**

```
Klassischer Fehler bei Shunt-Messung:

FALSCH:
  VBUS ──┬─── Last
         │
       [SHUNT]
         │
         ├─── INA+ Sense
         ├─── INA- Sense  ← Gemeinsamer Knoten!
         │
       GND ───

Problem: Spannungsabfall in PCB-Traces fließt in Messung ein!

RICHTIG (Kelvin / 4-Wire):
  VBUS ────┬─── Last (Power)
           │
         [SHUNT]
           │
           ├──────────── INA+ Sense (dünner Trace)
           │
           ├──────────── INA- Sense (dünner Trace)
           │
         ──┴─── GND (Power)

Regel: Sense-Leitungen DIREKT an Shunt-Pads, keine anderen Verbindungen!
```

**2. Trace-Widerstände und Thermoelektrik**

```
Problem:
- Kupfer-Traces haben Widerstand: ~0.5mΩ/mm bei 0.5mm Breite
- Bei 1A: 0.5mV/mm Spannungsabfall!
- Kupfer TCR: 3900 ppm/°C → Massiver Tempco!

Lösung für v2.4:
- INA+ Sense: Dünner Trace (0.15-0.2mm), direkt am Shunt-Pad
- INA- Sense: Dünner Trace (0.15-0.2mm), direkt am Shunt-Pad
- Power-Pfad: Breiter Trace (2-3mm), KEIN gemeinsamer Knoten mit Sense!
- Sense-Traces KURZ halten (<10mm)
```

**3. GND-Plane Rückströme**

```
Problem:
- Messstrom und Logik-GND teilen sich GND-Plane
- Spannungsabfall in Plane → Offset

Lösung:
- SEPARATE GND-Bereiche:
  * PGND (Power GND): Shunt, Power-Pfad
  * AGND (Analog GND): INA228
  * DGND (Digital GND): ESP32, Logic
- Verbindung an EINEM Punkt (Sternpunkt)
- INA228 direkt an AGND, nah am Shunt-AGND-Punkt
```

**4. Thermoelektrische Effekte (Seebeck-Effekt)**

```
Problem:
- Kupfer-PCB + Lötzinn = verschiedene Metalle
- Temperaturgradienten → Thermospannung
- Typisch: 3-40µV/°C

Kritische Stellen:
- Shunt-Lötstellen
- INA228-Lötstellen an Sense-Pins
- Via-Übergänge (Cu → Sn)

Minimierung:
- Symmetrisches Layout (beide Sense-Pfade identisch)
- Shunt und INA228 auf gleicher Temperatur
- Thermische Kopplung verbessern
- Keine Vias in Sense-Traces (wenn möglich)
```

---

## 🔧 KONKRETE LAYOUT-VERBESSERUNGEN für v2.4

### **1. INA228 Messschaltung - Optimiertes Layout**

```
Ideales Layout (Top View):

                 VBUS_IN ═══╗ (3mm breit)
                            ║
                    ┌───────╨───────┐
                    │   R_SHUNT     │ 10mΩ, Low-TCR, 2512
                    │   (2512)      │
                    └───┬───────┬───┘
                        │       │
                INA+────┤       ├────INA-  (0.2mm Traces, <10mm)
                        │       │
                        ║       ║
                     VBUS_OUT ═╩═══ Last (3mm breit)

Layer-Struktur:
- Top: Shunt, INA228, kurze Sense-Traces
- GND-Plane: Aufgetrennt (PGND / AGND), Verbindung an Sternpunkt
- Power: VBUS_IN, VBUS_OUT breit routen (2-3mm)

Kritische Abstände:
- Shunt ↔ INA228: <15mm
- Sense-Traces: 0.15-0.2mm breit, direkt am Shunt-Pad
- Power-Traces: >1mm Abstand zu Sense-Traces
- Keine thermischen Reliefs an Shunt-Pads (volle Kupferfläche)
```

**Design Rules für INA228-Bereich:**

```
1. Shunt-Platzierung:
   - Auf thermisch stabiler Zone (weg von MP4560, ESP32)
   - Gute Luftströmung
   - Symmetrisch zwischen VBUS_IN und VBUS_OUT

2. Sense-Traces:
   - 0.15-0.2mm Breite (minimaler Tempco-Einfluss)
   - KEINE Vias (wenn möglich)
   - Falls Vias nötig: Thermische Entkopplung
   - Identische Länge für INA+ und INA-
   - Parallele Führung (gleiche Temp-Exposition)

3. Guard-Ring (optional, aber empfohlen):
   - GND-Ring um Sense-Traces (0.3mm Abstand)
   - Verhindert Leckströme
   - Bessere EMI-Immunität

4. Thermal Management:
   - Kupferfläche unter Shunt minimieren (nur Pads)
   - INA228: Normale Thermal Reliefs
   - Temperatur-Sensor in Nähe platzieren (falls vorhanden)
```

---

### **2. Power Supply - Verbesserungen**

#### **MP4560DN (U1) - Step-Down Converter**

**Aktuelle Probleme:**

```
1. EMI/Noise auf 3V3-Rail
2. Mögliche Einkopplung in INA228
3. Suboptimale Entkopplung
```

**Verbesserungen für v2.4:**

**A) Input-Entkopplung (VIN):**

```
Aktuell: Vermutlich C4 (4.7µF) + C5 (10µF)

Verbessert:
- C_bulk: 22µF Keramik (X7R, 1206, 25V) - Murata GRM32ER71E226KE15L
- C_hf:   4.7µF Keramik (X7R, 0805, 25V) - Murata GRM21BR71E475KA12L  
- C_uhf:  100nF Keramik (X7R, 0402, 25V) - Murata GRM155R71E104KA87D

Platzierung:
- Alle 3 parallel
- C_uhf direkt am VIN-Pin (<2mm)
- C_hf <5mm vom VIN-Pin
- C_bulk <10mm vom VIN-Pin
```

**B) Output-Entkopplung (5V Rail):**

```
Aktuell: Vermutlich C6 (10µF)

Verbessert:
- C_bulk: 22µF Keramik (X7R, 1206, 10V) - Murata GRM32ER71A226KE15L
- C_hf:   10µF Keramik (X7R, 0805, 10V) - Murata GRM21BR71A106KE51L
- C_uhf:  100nF Keramik (X7R, 0402, 10V) - Murata GRM155R71A104KA01D

Zusätzlich für INA228-Versorgung:
- LC-Filter zwischen 5V und INA_VDD:
  * L: Ferrite Bead 600Ω@100MHz (Murata BLM18PG601SN1D)
  * C: 10µF + 100nF am INA228 VDD-Pin
```

**C) Layout-Optimierung:**

```
1. Induktor L1 (47µH):
   - Direkt am SW-Pin (<3mm)
   - Minimale Loop-Fläche: VIN → L1 → SW → GND
   - Abschirmung durch GND-Fläche

2. Bootstrap-Kondensator:
   - Direkt an BST und SW Pins (<2mm)

3. Feedback-Netzwerk:
   - FB-Trace dünn (0.2mm) und kurz
   - Weg von Schalt-Nodes (SW)
   - Keine Parallelführung zu SW-Trace

4. GND-Verbindung:
   - Mehrere Vias unter MP4560 (thermisches + elektrisches GND)
   - PGND-Bereich für Schaltregler
   - Separation zu AGND (INA228)
```

---

#### **ME6211C33 (U2) - LDO 3.3V**

**Verbesserungen:**

```
1. Input-Caps:
   Aktuell: C12 (10µF)
   
   Verbessert:
   - 10µF + 100nF parallel
   - Platzierung: <5mm von VIN-Pin

2. Output-Caps:
   Aktuell: C13 (10µF)
   
   Verbessert (ESP32 ist kritisch!):
   - 22µF X7R (Bulk, 1206)
   - 10µF X7R (Medium, 0805)
   - 4.7µF X7R (Schnell, 0603)
   - 100nF X7R (HF, 0402)
   
   Platzierung:
   - 100nF direkt am ESP32 VDD (<2mm) - MEHRERE!
   - 4.7µF <5mm vom ESP32
   - 10µF + 22µF <15mm vom ESP32
   - Sternförmige Verteilung um ESP32

3. ESP32 Power Integrity:
   - JEDES VDD-Pin: 100nF direkt (<2mm)
   - VDD_SPI: Extra 4.7µF + 100nF
   - VDD_RF: Separate Filterung (siehe RF-Verbesserungen)
   - Kurze, breite Power-Traces (>0.5mm)
   - Viele GND-Vias um ESP32 (thermisch + elektrisch)
```

---

#### **TPS2115 (U3) - Power Mux**

**Neben C11 (47µF Tantal):**

```
1. Zusätzliche Entkopplung:
   - VOUT: 100nF Keramik direkt am Pin (HF-Noise)
   - VIN1: 100nF Keramik
   - VIN2: 100nF Keramik
   - Alle <3mm von Pins

2. PR1/PR2 Pull-Resistors:
   - Low-Value (100kΩ → 10kΩ)
   - Schnellere Umschaltung
   - Weniger anfällig für Noise

3. ILIM-Netzwerk:
   - Präzisions-Widerstand (±1%)
   - Temperaturstabil (TCR <100ppm/°C)

4. Layout:
   - Kurze Verbindungen VOUT → C11
   - C11 direkt am VOUT-Pin (<5mm)
   - Tantal + Keramik parallel (ESR-Optimierung)
```

---

### **3. Signal-Integritäts-Verbesserungen**

#### **I2C-Bus (ESP32 ↔ INA228)**

```
Aktuelle Probleme:
- Mögliche EMI von MP4560
- Lange Traces anfällig für Störungen
- Keine Pull-Up-Filterung

Verbesserungen:

1. Pull-Up-Widerstände:
   Aktuell: Vermutlich 4.7kΩ oder 10kΩ
   
   Optimiert:
   - 2.2kΩ (schneller, bessere Noise-Immunity)
   - Serie-Ferrites (optional):
     * 100Ω @ 100MHz zwischen Pull-Up und Trace
     * Murata BLM18BB101SN1D
   - Pull-Up direkt an 3V3_CLEAN (gefiltert)

2. Trace-Routing:
   - Parallele Führung SDA/SCL (identische Länge ±5mm)
   - Abstand zu Power-Traces >1mm
   - Kreuzung mit SW-Node vermeiden (90° wenn nötig)
   - Guard-Traces mit GND (optional)

3. ESD-Schutz (falls extern zugänglich):
   - TVS-Dioden: PESD5V0S1BL (SOD-523)
   - An SDA, SCL zu GND
   - Direkt am Stecker

4. Terminierung (nur bei langen Traces >30cm):
   - Serie-Resistors: 33Ω an SDA, SCL (Quelle)
   - Reduziert Reflections
```

---

#### **USB-Datenleitung (falls verwendet)**

```
Falls D+/D- für USB-Kommunikation genutzt:

1. Differentielle Impedanz:
   - Ziel: 90Ω ±10%
   - Trace-Paar: Breite/Abstand berechnen
   - Beispiel (2-Layer, FR4):
     * w = 0.4mm, s = 0.25mm → Z_diff ≈ 90Ω

2. Längenabstimmung:
   - D+ und D- Länge identisch (±0.5mm)
   - Serpentinen für Matching

3. Schutz:
   - ESD-TVS: USBLC6-2SC6 (USB 2.0)
   - Direkt am USB-Stecker

4. GND-Fläche:
   - Durchgehend unter D+/D-
   - Keine Slots/Unterbrechungen
   - Vias (GND-Stitching) beidseitig
```

---

### **4. Thermisches Management**

```
1. Hotspots identifiziert:
   - MP4560DN (Schaltregler)
   - Shunt-Widerstand (I²R Heating)
   - ESP32-C3 (WiFi TX)
   - TPS2115 (bei hohen Strömen)

2. Verbesserungen:

   A) Copper Pour Optimization:
      - Große GND-Flächen für Wärmeabfuhr
      - Thermal Vias unter ICs (3x3 Array, 0.3mm)
      - MP4560: Besonders wichtig (6-9 Vias)

   B) Component Spacing:
      - Hotspots mind. 10mm Abstand
      - Luftstrom-Optimierung (falls Gehäuse)
      - INA228 weg von MP4560 (>20mm)

   C) Shunt Thermal Design:
      - Minimale Kupferfläche (nur Pads)
      - Freier Luftraum um Shunt
      - KEINE Thermal Reliefs an Pads
      - Symmetrische Wärmeabfuhr

   D) Temperature Monitoring (optional):
      - NTC oder Temp-Sensor nah am Shunt
      - Software-Kompensation der INA228-Messung
      - Thermische Warnung bei Überlast
```

---

## 🎯 PRIORITÄTEN für v2.4

### **HOCH (Kritisch für Genauigkeit):**

1. ✅ **INA228 Offset-Korrektur:**
    - Low-TCR Shunt-Widerstand (±20 ppm/°C)
    - Kelvin-Verbindung optimieren
    - Symmetrisches Sense-Trace Layout
    - PGND/AGND Separation

2. ✅ **TPS2115 Entkopplung:**
    - C11: 47µF Tantal + 100nF Keramik parallel

3. ✅ **INA228 Versorgungsspannung:**
    - Separate Filterung mit Ferrite Bead
    - 10µF + 100nF am VDD-Pin

---

### **MITTEL (Performance-Verbesserung):**

4. ✅ **MP4560 EMI-Reduktion:**
    - Verbesserte Input/Output-Entkopplung
    - Layout-Optimierung (Loop-Flächen)

5. ✅ **ESP32 Power Integrity:**
    - Mehrere 100nF Caps an allen VDD-Pins
    - Zusätzliche Bulk-Caps (22µF + 10µF + 4.7µF)

6. ✅ **I2C Bus Robustheit:**
    - 2.2kΩ Pull-Ups
    - Optimiertes Routing

---

### **NIEDRIG (Nice-to-Have):**

7. ⚪ **Thermal Management:**
    - Mehr Thermal Vias
    - Bessere Component Spacing

8. ⚪ **Test Points:**
    - VBUS_IN, VBUS_OUT (für Shunt-Messung)
    - 3V3, 5V (Versorgungsspannungen)
    - SDA, SCL (I2C Debug)
    - GND (mehrere Punkte)

9. ⚪ **ESD Protection:**
    - Falls externe Anschlüsse vorhanden

---

## 📐 LAYOUT-CHECKLIST für v2.4

```
INA228 Messbereich:
[ ] Shunt: Low-TCR Typ (≤20 ppm/°C)
[ ] Kelvin-Sense-Traces: 0.15-0.2mm breit, <10mm lang
[ ] Sense-Traces direkt an Shunt-Pads (kein gemeinsamer Knoten)
[ ] Power-Traces: 2-3mm breit, kein Überlapp mit Sense
[ ] INA228 <15mm vom Shunt
[ ] AGND separate von PGND
[ ] Symmetrisches Layout (INA+ und INA- identisch)
[ ] Keine Vias in Sense-Traces
[ ] Guard-Ring um kritische Traces (optional)

Power Supply:
[ ] MP4560: Minimale Loop-Flächen
[ ] L1 direkt am SW-Pin
[ ] Input: 22µF + 4.7µF + 100nF
[ ] Output: 22µF + 10µF + 100nF
[ ] Ferrite Bead vor INA228 VDD

TPS2115:
[ ] C11: 47µF Tantal + 100nF Keramik parallel
[ ] 100nF an VIN1, VIN2, VOUT

ESP32:
[ ] 100nF an JEDEM VDD-Pin (<2mm)
[ ] 4.7µF + 100nF an VDD_SPI
[ ] Bulk: 22µF + 10µF nahe ESP32
[ ] Viele GND-Vias um ESP32

I2C:
[ ] 2.2kΩ Pull-Ups
[ ] Parallele Führung SDA/SCL
[ ] Längenabgleich ±5mm
[ ] Abstand zu Switching-Nodes >1mm

Thermal:
[ ] Vias unter allen Power-ICs
[ ] INA228 >20mm von MP4560
[ ] Minimale Kupferfläche unter Shunt
[ ] Test Points für Debug
```

---

## 🧪 TEST & VALIDIERUNG

**Nach v2.4 Prototyp-Build:**

```
1. Offset-Messung:
   - Leerlauf bei verschiedenen Temperaturen messen
   - Ziel: <1mA Offset über 0-60°C

2. Genauigkeit:
   - Kalibrierter Last-Widerstand (±1%)
   - Vergleichsmessung mit Präzisions-Multimeter
   - 10mA, 100mA, 1A, 3A, 5A testen
   - Ziel: <1% Fehler über gesamten Bereich

3. TPS2115 Umschaltung:
   - Oszilloskop an VOUT
   - VBUS ↔ BATT Umschaltung
   - Spannungseinbruch messen
   - Ziel: <100mV Dip, <50µs Recovery

4. EMI:
   - Spektrum-Analyzer
   - MP4560 Switching Harmonics
   - I2C Signal-Qualität (Eye-Diagram)
   - ESP32 WiFi Performance

5. Thermal:
   - IR-Kamera oder Thermoelement
   - Bei Max-Last (3-5A)
   - 30min Dauerbetrieb
   - Ziel: Shunt <80°C, MP4560 <100°C
```

---

## 💡 ZUSÄTZLICHE FEATURES (Optional)

```
1. Auto-Kalibrierung:
   - INA228 Offset-Register nutzen
   - Software-Kalibrierung beim Startup
   - Temperatur-Kompensation

2. Over-Current Protection:
   - INA228 ALERT-Pin verwenden
   - Threshold programmierbar
   - TPS2115 Disable bei Überstrom

3. Logging:
   - Min/Max/Avg Werte
   - Energie-Verbrauch (Wh)
   - Temperatur-Logging

4. Display/UI:
   - OLED für lokale Anzeige
   - BLE für Remote-Monitor
   - Grafische Darstellung (Power vs Time)
```

---

## 📋 ZUSAMMENFASSUNG

**Hauptprobleme identifiziert:**

1. ✅ INA228 Offset: **Shunt-TCR + Layout**
2. ✅ TPS2115 Umschaltung: **C11 = 47µF Tantal (korrekt!)**

**Wichtigste Verbesserungen für v2.4:**

1. **Low-TCR Shunt** (±20 ppm/°C)
2. **Kelvin-Layout optimieren** (Sense-Traces, AGND/PGND)
3. **INA228 VDD filtern** (Ferrite + Caps)
4. **C11 = 47µF Tantal + 100nF** (parallel)
5. **ESP32 Power Integrity** (mehr Caps)
6. **MP4560 EMI-Reduktion** (Layout + Entkopplung)

**Erwartete Verbesserungen:**

- Offset: **4-8mA → <1mA**
- Genauigkeit: **±5% → ±1%**
- Temperaturstabilität: **200ppm/°C → 50ppm/°C**
- Umschalt-Glitch: **>200mV → <100mV**

---

Viel Erfolg mit v2.4! 🚀
