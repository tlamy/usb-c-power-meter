//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//
#include "Bluetooth.h"
#include "Display.h"
#include "PowerSensor.h"
#include "SerialData.h" // Include the new Serial.h for the Serial class
#include <Arduino.h>

#define INA_I2C_ADDRESS 0x41
#define RELEASE_VERSION "2.3.0"
#define SHUNT_RESISTANCE 0.010
#define MAX_CURRENT 10.0
#define DIAG_PIN 10
#define DEBUG_INA 0
#define DEBUG_BLE 0
#define ENABLE_SERIAL_OUT 1
#define ENABLE_BLE_OUT 1

#define SERVICE_UUID "01bc9d6f-5b93-41bc-b63f-da5011e34f68"
#define CHARACTERISTIC_UUID "307fc9ab-5438-4e03-83fa-b9fc3d6afde2"

// Create instances
SerialData serialOutput(Serial);
Display *display;
uint32_t chipId = ESP.getEfuseMac();
String deviceName = "MacWake PowerMeter " + String(chipId & 0xffff, HEX);
Bluetooth bluetooth(deviceName.c_str(), SERVICE_UUID, CHARACTERISTIC_UUID);
PowerSensor *powerSensor;

#include <vector>
#include <utility>

void scanI2C(bool diagnostics = false, std::vector<std::pair<uint8_t, String>> *foundDevices = nullptr) {
  Serial.println(diagnostics ? "--- I2C Diagnostic Scan ---" : "Scanning I2C bus...");
  int nDevices = 0;

  for (uint8_t address = 0x01; address < 0x7f; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      String name = "Unknown Device";
      if (address == INA_I2C_ADDRESS) name = "INA228";
      else if (address == 0x3C || address == 0x3D) name = "OLED";
      
      Serial.printf("Device found at 0x%02x", address);
      if (diagnostics) {
        Serial.printf(" [%s]", name.c_str());
      }
      Serial.println();
      
      if (foundDevices) {
        foundDevices->push_back({address, name});
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
  bool displayFound = false;
  for (const auto& dev : foundDevices) {
    if (dev.first == 0x3C || dev.first == 0x3D) {
      displayFound = true;
      break;
    }
  }

  if (displayFound) {
    if (!display) display = new Display();
    display->begin(SDA_PIN, SCL_PIN);
    display->showDiagnostics(RELEASE_VERSION, SHUNT_RESISTANCE, MAX_CURRENT, foundDevices);
  }

  Serial.println("*************************************\n");
  delay(5000); // Give time to read
}

void setup() {
  pinMode(DIAG_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  delay(100); // Wait for Serial and Pin stability

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

  // Configure power sensor
  powerSensor->configure(MAX_CURRENT, SHUNT_RESISTANCE);

  // scanI2C();

  // Initialize Bluetooth
  if (!bluetooth.begin()) {
    Serial.println("Failed to initialize Bluetooth!");
    // Continue without BLE
  }

  // Show splash screen
  Serial.println("Showing splash");
  display->splash(RELEASE_VERSION);
}

void loop() {
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
  serialOutput.out_pld( measurement);
#endif

#if ENABLE_BLE_OUT
  bluetooth.sendData(measurement);
#endif
#endif

  // Update display
  display->display_measurements(measurement);

  //delay(10);
}
