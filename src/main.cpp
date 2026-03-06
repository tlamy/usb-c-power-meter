//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//
#include <Arduino.h>
#include <Preferences.h>

#include "Bluetooth.h"
#include "Display.h"
#include "PowerSensor.h"
#include "SerialConsole.h"
#include "SerialData.h"  // Include the new Serial.h for the Serial class

#define INA_I2C_ADDRESS 0x41
#define RELEASE_VERSION "2.3.1"
#define SHUNT_RESISTANCE 0.010
#define MAX_CURRENT 16.0
#define DIAG_PIN 10
#define DEBUG_INA 0
#define DEBUG_BLE 0
#define ENABLE_SERIAL_OUT 1
#define ENABLE_BLE_OUT 1

#define SERVICE_UUID "01bc9d6f-5b93-41bc-b63f-da5011e34f68"
#define CHARACTERISTIC_UUID "307fc9ab-5438-4e03-83fa-b9fc3d6afde2"

// Create instances
SerialData serialOutput(Serial);
SerialConsole console(Serial);
Display* display;
uint32_t chipId = ESP.getEfuseMac();
String deviceName = "MacWake PowerMeter " + String(chipId & 0xffff, HEX);
Bluetooth bluetooth(deviceName.c_str(), SERVICE_UUID, CHARACTERISTIC_UUID);
PowerSensor* powerSensor;

#include <map>
#include <utility>
#include <vector>

static const std::map<uint8_t, String>& i2cDeviceNames() {
  static const std::map<uint8_t, String> names = {
      // OLED displays (SSD1306, SH1106, etc.)
      {0x3C, "Display"},
      {0x3D, "Display"},
      // INA power sensors
      {0x40, "Sensor"},  // INA228/INA226 A1=0, A0=0
      {0x41, "Sensor"},  // INA228/INA226 A1=0, A0=1
      {0x44, "Sensor"},  // INA228/INA226 A1=1, A0=0
      {0x45, "Sensor"},  // INA228/INA226 A1=1, A0=1
      {0x48, "Sensor"},  // INA3221 A0=GND
      {0x49, "Sensor"},  // INA3221 A0=VS
      {0x4A, "Sensor"},  // INA3221 A0=SDA
      {0x4B, "Sensor"},  // INA3221 A0=SCL
  };
  return names;
}

void scanI2C(bool diagnostics = false, std::vector<std::pair<uint8_t, String>>* foundDevices = nullptr) {
  Serial.println(diagnostics ? "--- I2C Diagnostic Scan ---" : "Scanning I2C bus...");
  int nDevices = 0;

  for (uint8_t address = 0x01; address < 0x7f; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      auto entry = i2cDeviceNames().find(address);
      String name = (entry != i2cDeviceNames().end()) ? entry->second : "Unknown Device";

      Serial.printf("Device found at 0x%02x", address);
      if (diagnostics) {
        Serial.printf(" [%s]", name.c_str());
      }
      Serial.println();

      if (foundDevices != nullptr) {
        foundDevices->emplace_back(address, name);
      }
      nDevices++;
    }
  }

  Serial.printf("I2C scan complete. Found %d device(s).\n", nDevices);
}

void runDiagnostics() {
  Serial.println("\n********** DIAGNOSTIC MODE **********");
  Serial.printf("Firmware Version: %s\n", RELEASE_VERSION);
  Serial.printf("Shunt Resistor: %.3f Ohm\n", SHUNT_RESISTANCE);
  Serial.printf("Max Current: %.1f A\n", MAX_CURRENT);

  std::vector<std::pair<uint8_t, String>> foundDevices;
  scanI2C(true, &foundDevices);

  // Also output to Display if available
  static constexpr uint8_t kOledAddresses[] = {0x3C, 0x3D};
  uint8_t displayAddr = 0;
  for (const auto& dev : foundDevices) {
    for (uint8_t addr : kOledAddresses) {
      if (dev.first == addr) {
        displayAddr = addr;
        break;
      }
    }
    if (displayAddr != 0) { break; }
  }

  if (displayAddr != 0) {
    if (display == nullptr) {
      display = new Display();
    }
    display->begin(SDA_PIN, SCL_PIN, displayAddr);
    display->showDiagnostics(RELEASE_VERSION, SHUNT_RESISTANCE, MAX_CURRENT, foundDevices);
  }

  Serial.println("*************************************\n");
  delay(5000);  // Give time to read
}

static constexpr uint16_t kPrefsNotSet = 0xFFFF;
static constexpr const char* kPrefsNamespace = "ina228";

static void applyStoredCalibration() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/true);
  const uint16_t shuntCal = prefs.getUShort("shunt_cal", kPrefsNotSet);
  const uint16_t tempCoeff = prefs.getUShort("temp_coeff", kPrefsNotSet);
  const float shuntOffset = prefs.getFloat("shunt_off_mv", 0.0F);
  prefs.end();

  INA228& ina = powerSensor->getINA228();
  if (shuntCal != kPrefsNotSet) {
    ina.setShuntCal(shuntCal);
    Serial.printf("NVRAM: SHUNT_CAL = %u\n", shuntCal);
  }
  if (tempCoeff != kPrefsNotSet) {
    ina.setShuntTemperatureCoefficent(tempCoeff);
    Serial.printf("NVRAM: SHUNT_TEMP_COEFF = %u ppm\n", tempCoeff);
  }
  ina.setTemperatureCompensation(true);

  if (shuntOffset != 0.0F) {
    powerSensor->setShuntOffset(shuntOffset);
    Serial.printf("NVRAM: SHUNT_OFFSET = %.4f mV\n", shuntOffset);
  }
}

void setup() {
  pinMode(DIAG_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  delay(100);  // Wait for Serial and Pin stability

  if (digitalRead(DIAG_PIN) == LOW) {
    // We need I2C and WiFi partially up for diag info
    Wire.begin(SDA_PIN, SCL_PIN);
    runDiagnostics();
  }

  Serial.println();
  Serial.printf("MacWake USB-Power V%s\n", RELEASE_VERSION);
  // Initialize display
  // Serial.println("Initializing the display");
  display = new Display();
  display->begin(SDA_PIN, SCL_PIN);
  // Serial.println("Display done.");
  //  delay(250);

  // Initialize I2C first, before any other components
  Serial.printf("Initializing I2C with SDA=%d, SCL=%d\n", SDA_PIN, SCL_PIN);
  if (!Wire.begin(SDA_PIN, SCL_PIN)) {
    Serial.println("Failed to initialize I2C");
    delay(1000);
    // Continue without I2C
  }
  // Serial.println("I2C initialized");

  powerSensor = new PowerSensor(INA_I2C_ADDRESS, Wire, DEBUG_INA);

  // Initialize power sensor
  if (!powerSensor->begin()) {
    Serial.println("Failed to initialize PowerSensor!");
  }

  // Configure power sensor, then apply any stored calibration overrides
  powerSensor->configure(MAX_CURRENT, SHUNT_RESISTANCE);
  applyStoredCalibration();

  // scanI2C();

  // Initialize Bluetooth
  if (!bluetooth.begin()) {
    Serial.println("Failed to initialize Bluetooth!");
    // Continue without BLE
  }

  // Show splash screen
  Serial.println("Showing splash");
  display->splash(RELEASE_VERSION);

  // Register serial console commands
  console.registerCommand("scan", "scan I2C bus and list devices", [](const SerialConsole::Args&) { scanI2C(true); });
  console.registerCommand("measure", "print a single measurement",
                          [](const SerialConsole::Args&) { PowerSensor::printMeasurement(powerSensor->readMeasurement()); });
  console.registerCommand("m", "alias for measure",
                          [](const SerialConsole::Args&) { PowerSensor::printMeasurement(powerSensor->readMeasurement()); });
  console.registerCommand("diag", "run full diagnostics (I2C scan + display)",
                          [](const SerialConsole::Args&) { runDiagnostics(); });
  console.registerCommand("reset", "reboot the device", [](const SerialConsole::Args&) {
    Serial.println("Rebooting...");
    Serial.flush();
    ESP.restart();
  });
  console.registerCommand("tempcal", "show or set SHUNT_CAL / SHUNT_TEMP_COEFF  |  tempcal [cal <n> | coeff <n> | reset]",
                          [](const SerialConsole::Args& args) {
                            INA228& ina = powerSensor->getINA228();

                            if (args.size() == 1) {
                              // Show current register values + live readings
                              const PowerMeasurement m = powerSensor->readMeasurement();
                              Serial.printf("SHUNT_CAL:        %u\n", ina.getShuntCal());
                              Serial.printf("SHUNT_TEMP_COEFF: %u ppm\n", ina.getShuntTemperatureCoefficent());
                              Serial.printf("Temperature:      %.2f C\n", m.temperature_c);
                              Serial.printf("Current:          %.6f A\n", m.current);
                              return;
                            }

                            String sub = args[1];
                            sub.toLowerCase();

                            if (sub == "cal" && args.size() == 3) {
                              const uint16_t val = static_cast<uint16_t>(args[2].toInt());
                              ina.setShuntCal(val);
                              Preferences prefs;
                              prefs.begin(kPrefsNamespace, /*readOnly=*/false);
                              prefs.putUShort("shunt_cal", val);
                              prefs.end();
                              Serial.printf("SHUNT_CAL set to %u and saved to NVRAM.\n", val);

                            } else if (sub == "coeff" && args.size() == 3) {
                              const uint16_t val = static_cast<uint16_t>(args[2].toInt());
                              ina.setShuntTemperatureCoefficent(val);
                              Preferences prefs;
                              prefs.begin(kPrefsNamespace, /*readOnly=*/false);
                              prefs.putUShort("temp_coeff", val);
                              prefs.end();
                              Serial.printf("SHUNT_TEMP_COEFF set to %u ppm and saved to NVRAM.\n", val);

                            } else if (sub == "reset") {
                              Preferences prefs;
                              prefs.begin(kPrefsNamespace, /*readOnly=*/false);
                              prefs.remove("shunt_cal");
                              prefs.remove("temp_coeff");
                              prefs.end();
                              // Reapply firmware defaults
                              powerSensor->configure(MAX_CURRENT, SHUNT_RESISTANCE);
                              ina.setShuntTemperatureCoefficent(0);
                              ina.setTemperatureCompensation(true);
                              Serial.println("Calibration cleared from NVRAM, firmware defaults restored.");

                            } else {
                              Serial.println("Usage: tempcal [cal <n> | coeff <n> | reset]");
                            }
                          });

  console.registerCommand("zerocal", "calibrate shunt zero offset (remove load first)  |  zerocal [reset]",
                          [](const SerialConsole::Args& args) {
                            if (args.size() == 2 && args[1] == "reset") {
                              powerSensor->setShuntOffset(0.0F);
                              Preferences prefs;
                              prefs.begin(kPrefsNamespace, /*readOnly=*/false);
                              prefs.remove("shunt_off_mv");
                              prefs.end();
                              Serial.println("Shunt offset cleared.");
                              return;
                            }

                            // Take 10 raw measurements (offset cleared temporarily)
                            const float savedOffset = powerSensor->getShuntOffset();
                            powerSensor->setShuntOffset(0.0F);

                            Serial.println("Measuring shunt offset (10 samples)...");
                            float sum = 0.0F;
                            for (int i = 0; i < 10; i++) {
                              sum += powerSensor->getShuntMilliVolts();
                              delay(50);
                            }
                            const float offset = sum / 10.0F;

                            if (fabsf(offset) > 5.0F) {
                              Serial.printf("Offset %.4f mV out of range (>5 mV) — not saved. Check for load.\n", offset);
                              powerSensor->setShuntOffset(savedOffset);
                              return;
                            }

                            powerSensor->setShuntOffset(offset);
                            Preferences prefs;
                            prefs.begin(kPrefsNamespace, /*readOnly=*/false);
                            prefs.putFloat("shunt_off_mv", offset);
                            prefs.end();
                            Serial.printf("Shunt offset = %.4f mV saved.\n", offset);
                          });
}

void loop() {
  console.poll();

  // Handle Bluetooth connections
  bluetooth.handleConnections();

  // Read sensor data using PowerSensor class
  PowerMeasurement measurement = powerSensor->readMeasurement();

  if (!measurement.valid) {
    Serial.println("Failed to read power measurement");
    delay(100);
    return;
  }

  // Send data via configured protocols
#if !DEBUG_INA
#if ENABLE_SERIAL_OUT
  if (!console.isActive()) {
    serialOutput.out_pld(measurement);
  }
#endif

#if ENABLE_BLE_OUT
  bluetooth.sendData(measurement);
#endif
#endif

  // Update display
  display->display_measurements(measurement);

  // delay(10);
}
