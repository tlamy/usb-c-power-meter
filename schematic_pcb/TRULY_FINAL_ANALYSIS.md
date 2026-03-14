# USB-C Power Meter - TRULY FINAL ANALYSIS

**After Clarification of Actual Schematic Details**

---

## CRITICAL CORRECTIONS

### **Correction #1: Crystal Load Capacitors**

- **FACT:** C22 and C23 are BOTH 15pF (identical) ✓
- **MY ERROR:** I misread as C23=15pF and C25=1.2pF
- **STATUS:** No issue here. Design is correct.
- **ACTION:** None needed

---

### **Correction #2: Vbus Ferrite Bead Question**

You asked: **Which Vbus? Between which parts should I add a ferrite bead?**

This is THE critical question that makes me realize I was giving **bad advice for your application**.

**The Real Issue With My Recommendation:**

I suggested adding a ferrite bead to "filter charger switching noise" in the main power path. But you're asking the
right question: **Why would you add impedance to a 15A passthrough current measurement path?**

**Answer: You shouldn't.**

**Ferrite bead impedance at 15A:**

- A typical 100Ω@100MHz ferrite still has 10-50Ω at lower frequencies
- At 15A: I²R = 15² × 0.05Ω = 11.25W heating in the ferrite
- This creates voltage drop that corrupts your INA228 measurement
- Completely defeats the purpose of accurate power monitoring

**Your design philosophy is CORRECT:**

- Keep main power path **clean and low-impedance**
- Filter the measurement signal instead (which you're already doing)
- Let INA228 reject noise digitally

---

### **Correction #3: I2C Pull-up Resistors**

- **FACT:** R11, R12 = 3k3 (not 12k) ✓
- **MY ERROR:** Wrong component references AND wrong values
- **STATUS:** 3k3 is actually MORE aggressive than standard 10k
- **ANALYSIS:** 3k3 provides tighter pull-ups = faster rise time
- **BETTER FOR:** Short traces, faster I2C speeds (400kHz+)
- **ACTION:** No change needed (your value is actually good)

---

## THE BIGGER QUESTION: EMI IN THE POWER PATH

Your question was spot-on:

> "Is it wise to NOT mess around with EMI in the power path going through the device for measuring? I am already
> filtering noise from the shunt going into the INA228?"

**YES. You are RIGHT to be cautious about the power path.**

### **Why NOT to filter main Vbus:**

1. **Impedance corrupts measurement**
    - Ferrite = series impedance = voltage drop
    - INA228 measures across shunt: V = I × R_shunt
    - If main path voltage drops, your measurement is wrong
    - You're trying to measure accurately, not "clean" the path

2. **Charger switching noise is NORMAL and EXPECTED**
    - SMPS chargers inherently switch at 100-500kHz
    - This is not a problem to solve—it's the input signal
    - Your passthrough device should pass it through unchanged

3. **You're filtering at the RIGHT place**
    - Shunt voltage filtering (before INA228) is the smart approach
    - Analog filtering of the measurement signal is where it belongs
    - Not in the power path itself

4. **15A current with ferrite = heating**
    - Power dissipation in ferrite bead is wasted power
    - Device efficiency decreases
    - Thermal design becomes more complex

### **VERDICT: Don't add ferrite to main Vbus path** ✓

---

## WHAT ACTUALLY MATTERS FOR YOUR DESIGN

### **1. Crystal Oscillator EMI (Still relevant)**

**The REAL EMI problem:** Your 40MHz clock radiates RF across DC-500MHz

This affects:

- I2C bus immunity (RF couples into SDA/SCL)
- Measurement accuracy (RF on analog shunt sense lines)
- Not the main power path, but nearby analog circuits

**Relevant fixes:**

- Guard GND around crystal (layout only, $0.00)
- Series resistors on oscillator pins (33Ω, ~$0.10)
- Route I2C away from crystal area (layout only, $0.00)

**Why still important:** Clock jitter and RF radiation affect I2C timing and measurement circuits, not the passthrough
power measurement itself.

### **2. Shunt Measurement Filtering (You're doing it right)**

You stated: "I am already filtering noise from the shunt going into the INA228"

**Key questions to verify:**

- What's the filter topology? (RC lowpass? LC? INA228 internal filtering?)
- What's the cutoff frequency?
- Does it adequately reject charger switching harmonics?
- Are the filter components physically close to INA228?

If you've validated this filtering works, **you're done with measurement EMI.** The shunt filter is exactly where
filtering should happen.

### **3. TPS2115 OR-ing Supply (Probably fine)**

For powering the meter itself (ESP32 + INA228 3.3V rail):

- TPS2115 has internal current limiting
- Your decoupling caps on the meter supply should be adequate
- Meter draws ~100mA max, very low EMI contributor
- This is a low-power supply line, not the 15A passthrough

**Probably doesn't need additional filtering.**

---

## HONEST FINAL ASSESSMENT

### **Real Issues Worth Fixing:**

1. ✅ **Crystal oscillator RF radiation** (affects I2C, measurement)
    - Guard GND around Y1 (layout only, $0.00)
    - 33Ω series resistors on crystal pins ($0.10)
    - Effort: Layout review + schematic update
    - Impact: Cleaner I2C, better RF immunity on measurement

2. ✅ **I2C signal routing** (prevent RF coupling)
    - Keep SDA/SCL away from crystal
    - Keep SDA/SCL away from high dI/dt power traces
    - Effort: Layout check only
    - Impact: Better EMI immunity

### **NOT Actually Issues:**

❌ Ferrite on main Vbus (would corrupt measurement)  
❌ Crystal capacitors (C22/C23 @ 15pF each are correct)  
❌ I2C pull-ups (3k3 is fine, maybe even optimal)

---

## REVISED IMPLEMENTATION CHECKLIST

### **Do These (Layout Only):**

- [ ] Review crystal (Y1) layout
    - Is there guard GND around it?
    - Are INA228 sense lines nearby? (bad)
    - Are I2C lines nearby? (bad)

- [ ] Add GND guard ring around crystal if missing
    - 200mil trace width, vias every 2mm
    - Cost: $0.00, Time: 30 min layout

- [ ] Verify I2C routing
    - Are SDA/SCL on opposite side from crystal?
    - Are they separated from high dI/dt power traces?
    - Cost: $0.00, Time: 15 min review

- [ ] Optional: Add 33Ω resistors on crystal oscillator pins
    - Reduces RF radiation
    - Cost: $0.10, Time: 10 min schematic

### **Don't Do These:**

- ❌ Don't add ferrite to main Vbus path
- ❌ Don't change crystal load capacitors (correct as-is)
- ❌ Don't change I2C pull-ups (3k3 is good)

---

## QUESTIONS FOR YOU

Before final recommendation, clarify:

1. **Shunt filter topology:**
    - Are you using RC lowpass? What's the cutoff frequency?
    - Or relying on INA228 internal filtering?
    - How confident are you it rejects charger switching noise?

2. **I2C placement:**
    - Where is the INA228 I2C connector relative to crystal?
    - Are they on opposite sides of board?
    - Any crossover traces?

3. **High-current path:**
    - How are you handling the 15A copper layout?
    - Any thermal issues observed?
    - Voltage drop acceptable?

---

## FINAL VERDICT

**Your design is SMART because:**

- ✓ You keep main power path clean (correct for measurement)
- ✓ You filter at measurement point (correct location)
- ✓ You use TPS2115 OR-ing (clever power distribution)
- ✓ You recognized that main path filtering is risky

**Actual issues to address:**

- Crystal RF radiation (layout + optional 33Ω series R)
- I2C routing away from interference sources
- Confidence in shunt filter adequacy

**Effort required:** 1-2 hours layout review, $0.00-0.10 cost

**Why I was wrong initially:**

- Assumed power path filtering is always good (not true for measurement devices)
- Didn't read your schematic carefully (wrong component references)
- Didn't question the fundamental design philosophy (you were right)

Your design philosophy is **superior** to the standard "ferrite everything" approach I initially suggested.

