# USB-C Power Meter with WiFi/BLE - RF Design Essentials

**High-Current Passthrough + Wireless Transmission**

---

## THE REAL PROBLEM NOW

You have a **very challenging RF environment:**

1. **15A passthrough current** creates massive dI/dt (switching noise)
2. **Charger SMPS** switching at 100-500kHz ± harmonics to GHz range
3. **ESP32 WiFi/BLE transmission** at 2.4GHz (same as microwave oven!)
4. **Shunt measurement** needs nV-level noise rejection
5. **USB data lines** carrying PD negotiation signals

All of this on a **small PCB**, probably with **shared ground plane**.

This is not a casual design—this is **mixed-signal RF on a power meter**. Standard approaches won't cut it.

---

## CRITICAL RF ISSUES TO ADDRESS

### **Issue #1: 2.4GHz Switching Noise Coupling**

**The Problem:**

ESP32 switching regulator typically operates at ~2-4MHz internal clock. This generates:

- Fundamental: 2-4MHz
- Harmonics at: 6, 8, 10, 12MHz ... up to GHz range
- **40MHz harmonic:** 40 × 2MHz = 80MHz (far from 2.4GHz, but close in log scale)
- **60th harmonic:** 60 × 40MHz = 2.4GHz (DIRECTLY IN WIFI BAND!)

**Impact:**

- WiFi receiver noise floor rises (harder to receive weak signals)
- BLE sensitivity degraded
- TX power may need to be higher (battery drain, EMI radiation)
- Shunt measurement could have 2.4GHz noise coupling in

**This is the biggest RF issue you face.**

### **Issue #2: High-Current Path Radiation**

**The Problem:**

15A through shunt = massive dI/dt transients. When charger switches:

- Current changes from 0A → 15A in ~100ns
- dI/dt = 15A / 100ns = **150 A/µs**
- This radiates like crazy across entire frequency spectrum
- Ground plane impedance = Z = L × dI/dt
- Even good GND plane has some inductance, creating voltage spikes

**Impact:**

- Antenna sees variable ground potential
- TX pattern becomes non-linear, spreads spectrum
- RX antenna picks up switching noise (desense)
- EMI to nearby circuits

### **Issue #3: Antenna Placement + Ground**

**The Problem:**

Where is your antenna? If it's on the same PCB:

- Antenna impedance: nominally 50Ω
- Antenna return current through GND plane
- If GND plane is noisy (15A transients), antenna sees poor reference
- Radiation pattern becomes unpredictable
- Antenna efficiency degrades

**Impact:**

- TX range reduced
- RX sensitivity reduced
- Increased power consumption (trying to overcome poor coupling)

---

## REQUIRED DESIGN CHANGES

### **Change #1: Separate GND Planes (CRITICAL)**

**What you need:**

- **GND Plane 1 (Analog GND):** RF circuitry + antenna + ESP32 RF section
- **GND Plane 2 (Digital GND):** High-current path + digital logic
- **Single point star connection** between them at one location only

**Why this matters:**

Switching noise from 15A path doesn't contaminate RF GND plane. Each signal sees clean reference.

**Implementation:**

1. Split PCB into two GND regions (top/bottom or left/right)
2. **Physical separation:** High-current path on one side, antenna/RF on other
3. **Single connection point:** Use single wide trace or via connecting the two planes
    - Location: Near power distribution (TPS2115 area)
    - This is your **star point** for EMI control

4. **Via separation:**
    - Analog GND vias: cluster near antenna/ESP32 RF pins
    - Digital GND vias: cluster near shunt and high-current path

**Cost:** $0.00 (layout change only)
**Benefit:** -20dB to -30dB EMI reduction between RF and power domains
**Complexity:** Medium (requires PCB redesign)

### **Change #2: Dedicated RF Supply Plane**

**What you need:**

ESP32 3.3V supply should have **two separate decoupling capacitor groups:**

**Group A - Analog RF Supply (for WiFi/BLE transceiver):**

- Location: Very close to ESP32 (within 5mm of RF section)
- 10µF low-ESR ceramic (X7R, 10V+) - for WiFi switching transients
- 1µF ceramic (X7R) - for BLE transients
- 0.1µF ceramics (2-3 pieces) - HF decoupling
- **Total: ~4-5 capacitors dedicated to RF supply**
- All vias connect directly to **Analog GND plane**

**Group B - Digital Supply (for ESP32 digital logic):**

- Location: Near ESP32 digital pins
- 10µF ceramic (X7R) - digital supply bulk
- 1µF ceramics (2-3) - transient filtering
- 0.1µF ceramics (2-3) - logic supply bypass
- All vias connect to **Digital GND plane**

**Why separate groups:**

- WiFi switching current (100-200mA spikes) is isolated from digital logic
- Reduces coupling of RF noise back into I2C/measurement circuits
- Clean supply for RX comparators (improves sensitivity)

**Cost:** +$0.20-0.30 (additional bypass capacitors)
**Benefit:** +3-5dB RX sensitivity improvement, cleaner measurements
**Complexity:** Low (just more capacitors, but correct placement critical)

### **Change #3: Antenna Isolation**

**What you need:**

1. **Antenna location:** Place on edge of PCB, away from high-current path
    - Ideally opposite side from shunt
    - Minimum 50mm from main power traces

2. **Antenna feedline:**
    - Route antenna trace on layer closest to **Analog GND plane**
    - Keep trace SHORT (< 50mm if possible)
    - Width to maintain ~50Ω impedance (check footprint spec)
    - No high-speed signals near antenna trace

3. **Antenna GND return:**
    - Antenna ground pad connects ONLY to Analog GND plane
    - Multiple vias (3-5) around antenna pad
    - Do NOT connect to main digital GND

4. **Shielding (optional but recommended):**
    - Draw GND trace ring around antenna area (like crystal guard)
    - Not a Faraday cage (leave connection to GND plane)
    - Just keeps stray high-frequency noise away

**Cost:** $0.00 (layout only)
**Benefit:** Improved RX/TX consistency, predictable radiation pattern
**Complexity:** Medium

### **Change #4: WiFi Transient Power Handling**

**The Problem:**

WiFi TX current spikes to 150-200mA in microseconds. Your TPS2115 + single bulk cap design might not handle this.

**Symptom you might see:**

- Voltage sag during WiFi TX
- I2C clock stretching (device not responding during TX)
- Corrupted INA228 readings
- Erratic BLE operation

**Solution - Add dedicated WiFi bulk capacitor:**

1. **Location:** Directly on ESP32 3.3V input (between TPS2115 and ESP32 Vcc pins)
2. **Value:** 10µF, 10V ceramic, **low-ESR** (< 50mΩ)
    - Example: Samsung CL10B106MQ8NNNC (10µF, 10V, low-ESR)
    - Cost: ~$0.10-0.15
3. **GND connection:** Direct to Analog GND plane (shortest possible via)

**Why this capacitor specifically:**

- WiFi spike lasts ~100µs
- Standard capacitor ESR causes voltage droop
- Low-ESR cap absorbs spike with minimal voltage change
- Transient current: I = C × dV/dt
- With 10µF + 50mΩ ESR: voltage droop ≈ (150mA × 100µs / 10µF) + (150mA × 50mΩ) = 1.5V + 7.5mV = ~1.5V sag
- Without this cap, you might see 2-3V sag (brown-out territory)

**Cost:** +$0.10-0.15
**Benefit:** Critical for stable WiFi operation
**Complexity:** Low

### **Change #5: Shunt Measurement RF Filtering**

**What you need:**

You said you're filtering shunt noise. For 2.4GHz WiFi in same device, this becomes critical.

**Recommended shunt filter (if not already present):**

```
Vin+ (to INA228) ━━[R1=1k]━━┳━━ to INA228 Vin+ pin
                           │
                          [C1=10nF]
                           │
                          GND (Analog GND)

Vin- ━━━━━━━━━━━━━━━━━━━━━━┳━━ to INA228 Vin- pin
                           │
                          [C2=10nF]
                           │
                          GND (Analog GND)
```

**Why 1k + 10nF:**

- Cutoff frequency: f_c = 1/(2π × 1k × 10nF) ≈ 16kHz
- Rejects 2.4GHz WiFi noise completely
- Passes DC measurement accurately
- ESP32 internal clock (40MHz) also rejected

**Alternatives (depends on your current filter):**

If your filter has higher cutoff:

- Reduce R and/or increase C to lower f_c
- Target: f_c < 10kHz at minimum
- But f_c >> measurement bandwidth (probably Hz to low kHz)

**Cost:** +$0.05 (two small caps and resistors, probably already present)
**Benefit:** Eliminates 2.4GHz noise from WiFi RX/TX
**Complexity:** Low

### **Change #6: Crystal Oscillator Isolation (even MORE critical now)**

**Why it matters more with WiFi:**

- Crystal fundamental: 40MHz
- 60th harmonic: 2.4GHz (WIFI BAND!)
- If crystal RF leaks into antenna, it desenses RX
- If antenna RX noise couples back to crystal, it causes jitter

**Must do:**

1. **Guard GND around crystal** (200mil wide, vias every 2mm)
2. **33Ω series resistors** on crystal pins (not optional anymore)
3. **Separate crystal oscillation from antenna by > 100mm**
4. **Crystal sense lines shielded** if possible (GND guard traces on both sides)

**Cost:** +$0.10 (33Ω resistors)
**Benefit:** Prevents 2.4GHz leakage from clock, critical for WiFi
**Complexity:** Medium (requires layout care)

---

## PCB LAYOUT STRATEGY FOR RF DEVICE

### **Layer Stack (Recommended):**

If you have 4 layers:

```
Layer 1 (Top):    Antenna trace, ESP32 RF signals, analog circuits
Layer 2:          Analog GND plane (wifi/RF area) 
Layer 3:          Digital GND plane (power/digital area)
Layer 4 (Bottom): TPS2115, high-current traces, digital logic
```

If 2 layers only (harder, but doable):

```
Layer 1 (Top):    All signals (separate into analog vs digital regions)
Layer 2 (Bottom): Split GND (analog on one half, digital on other)
```

### **Signal Routing Priority:**

1. **Highest priority:** Antenna trace (must be clean, 50Ω impedance)
2. **High priority:** ESP32 RF pins (minimal crosstalk)
3. **High priority:** INA228 sense lines (minimal coupling from power)
4. **Medium priority:** I2C bus (away from RF)
5. **Medium priority:** Crystal oscillator (away from antenna)
6. **Lower priority:** Digital logic traces

### **Power Distribution:**

```
Charger VBUS
    ↓
  [Connector with multiple GND vias]
    ↓
  [Wide traces to TPS2115, < 10mm]
    ↓
  TPS2115
    ↓
  [Split here: RF supply vs Digital supply]
    ↓
  [10µF RF bulk cap] → [Analog GND plane]
  [10µF Digital bulk] → [Digital GND plane]
    ↓
  [1µF caps + 0.1µF caps distributed]
    ↓
  ESP32 (RF pins on Analog supply, Digital pins on Digital supply)
```

---

## RECOMMENDED COMPONENT ADDITIONS FOR WIFI/BLE

### **Must Add:**

1. **Dedicated RF bulk capacitor (10µF low-ESR)**
    - Cost: $0.10-0.15
    - Placement: On ESP32 Vcc, Analog GND
    - Part example: Samsung CL10B106MQ8NNNC
    - **This is not optional for stable WiFi TX**

2. **Shunt RF filter (if not present)**
    - 1k resistor + two 10nF caps
    - Cost: $0.03-0.05
    - Ensures 2.4GHz doesn't couple into INA228
    - **Critical for accurate measurement during WiFi**

### **Should Add:**

3. **Crystal series resistors (33Ω × 2)**
    - Cost: $0.10
    - Reduces 40MHz + harmonics radiation
    - **Important to prevent clock from desensing RX**

4. **Additional bypass caps for analog RF supply**
    - Extra 1µF and 0.1µF near ESP32 RF pins
    - Cost: $0.10-0.15
    - Improves WiFi stability
    - **Worthwhile if doing RF optimization**

### **Could Add (Conservative):**

5. **Ferrite bead on meter 3.3V supply**
    - 100Ω@100MHz (0603)
    - Cost: $0.15
    - Helps isolate digital switching from RF supply
    - **Useful but not critical if GND planes are separate**

### **Probably Not Needed:**

- ❌ D+/D- series resistors (still probably not worth the risk)
- ❌ Additional shielding (good PCB design > shielding cans)

---

## TESTING REQUIREMENTS

Once you build with WiFi/BLE, you MUST test:

### **RF Performance Tests:**

1. **WiFi/BLE Range**
    - Transmit at max power, measure range
    - Compare before/after optimization
    - Should see improvement after RF supply fixes

2. **RX Sensitivity**
    - Use WiFi analyzer app to measure signal strength
    - During charger operation (15A), check if sensitivity degrades
    - Should remain stable with proper GND separation

3. **Measurement Accuracy During WiFi**
    - INA228 readings while transmitting WiFi
    - Should not see spikes > 1-2 LSB when TX is active
    - Indicates shunt filter is working

4. **TX Stability**
    - Monitor WiFi TX power consumption
    - Should be consistent (not jumping up/down)
    - If TX power is erratic, supply is unstable

### **EMI Tests:**

5. **Radiated Emissions (50MHz-6GHz)**
    - FCC pre-scan before final production test
    - Peak limits: Class A < 109 dBµV/m, Class B < 99 dBµV/m
    - With proper GND separation, you should pass
    - Biggest risk: 2.4GHz WiFi TX leakage

6. **Clock Noise (40MHz ± harmonics)**
    - Spectrum analyzer on antenna feed
    - Should see WiFi peak, not crystal peak
    - If crystal harmonics visible, improve shielding

---

## HONEST ASSESSMENT

### **Your Challenge Level:**

🔴 **HIGH DIFFICULTY** - Not impossible, but requires careful design

**Why:**

- 15A switching + 2.4GHz RF = nightmare for noise coupling
- Small PCB = limited physical separation
- Measurement accuracy requirement = needs quiet analog ground
- WiFi/BLE = zero tolerance for supply noise

### **Effort Required:**

- **Schematic changes:** 2-3 hours (add capacitors, maybe ferrite)
- **PCB redesign:** 8-12 hours (GND plane split, antenna placement, routing)
- **Testing:** 4-6 hours (range test, measurement accuracy, EMI pre-scan)
- **Debugging:** 4-8 hours (if issues found)

**Total:** 18-30 hours engineering, probably 2-3 PCB revisions

### **Cost Impact:**

- Component additions: +$0.35-0.50 per unit
- PCB cost: Minimal (same size/layer count)
- Antenna cost: Depends on type (not listed in current BOM)

### **Risk Mitigation:**

1. **Prototype first** without WiFi/BLE to validate passthrough measurement
2. **Add WiFi in second revision** (cleaner design separation)
3. **Antenna selection critical** - integrated trace, chip antenna, or external monopole?
4. **Simulation recommended** (even basic 3D EM sim of GND planes + antenna)

---

## RECOMMENDATION

**For WiFi/BLE capability to work well:**

### **Must Do:**

1. ✅ **Split GND planes** (Analog RF vs Digital power)
    - This is your biggest win
    - Enables everything else to work

2. ✅ **Dedicated RF supply capacitor** (10µF low-ESR on Analog GND)
    - Non-negotiable for WiFi TX stability

3. ✅ **Shunt RF filter** (1k + 10nF, if not already present)
    - Essential for measurement accuracy during TX

4. ✅ **Crystal isolation** (guard GND + 33Ω series R)
    - Prevents clock harmonics from interfering with WiFi

### **Should Do:**

5. ⚠️ **Extra analog supply bypassing**
    - Additional 1µF + 0.1µF near ESP32 RF pins
    - Cost: $0.10-0.15

6. ⚠️ **Antenna placement strategy**
    - Opposite side from shunt/high-current path
    - Away from crystal oscillator

### **Cost & Effort:**

- **Cost:** +$0.35-0.50 per unit (reasonable)
- **Effort:** 8-12 hours PCB redesign (non-trivial)
- **Risk:** Medium (RF design has learning curve, but proven techniques)

**This is doable, but not a "small tweak" to your current design.** WiFi/BLE RF adds real constraints that require
thoughtful PCB architecture.

