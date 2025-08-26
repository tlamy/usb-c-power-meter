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
#define RELEASE_VERSION "2.0.0"
#define DEBUG_INA 0
#define DEBUG_BLE 1
#define ENABLE_SERIAL_OUT 0
#define ENABLE_BLE_OUT 1

#define SERVICE_UUID "01bc9d6f-5b93-41bc-b63f-da5011e34f68"
#define CHARACTERISTIC_UUID "307fc9ab-5438-4e03-83fa-b9fc3d6afde2"

// Create instances
SerialData serialOutput(Serial);
Display *display;
Bluetooth bluetooth("MacWake-USBPowerMeter", SERVICE_UUID, CHARACTERISTIC_UUID);
PowerSensor *powerSensor;

// Max current tracking for display
#define AMP_BUF_SIZE 32
uint8_t last_volts = 0;
int last_currents[AMP_BUF_SIZE];
uint8_t current_ptr = 0;
int max_current = 0;

void scanI2C() {
  Serial.println("Scanning I2C bus...");
  int nDevices = 0;

  for (uint8_t address = 0x01; address < 0x7f; address++) {
    Serial.printf("Checking address 0x%02x... ", address);
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.println("FOUND");
      nDevices++;
    } else {
      Serial.println("not found");
    }
  }

  Serial.printf("I2C scan complete. Found %d device(s).\n", nDevices);
}

int get_max_current(int current, uint8_t volts) {
  if (volts != last_volts) {
    current_ptr = 0;
    for (int i = 0; i < AMP_BUF_SIZE; ++i) {
      last_currents[i] = 0;
    }
    last_volts = volts;
    max_current = 0;
  }

  if (max_current > 0 && last_currents[current_ptr] == max_current) {
    last_currents[current_ptr] = current;
    max_current = 0;
    for (int i = 0; i < AMP_BUF_SIZE; ++i) {
      if (last_currents[i] > max_current)
        max_current = last_currents[i];
    }
  } else {
    last_currents[current_ptr] = current;
    if (current > max_current) {
      max_current = current;
    }
  }

  current_ptr = (current_ptr + 1) % AMP_BUF_SIZE;
  return max_current;
}

void setup() {
  Serial.begin(115200);
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
  powerSensor->configure(10.0, 10e-3); // 10A max, 10mΩ shunt

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
  serialOutput.out_pld(
    measurement); // Call the method on the serialOutput object
#endif

#if ENABLE_BLE_OUT
  bluetooth.sendData(measurement);
#endif
#endif

  // Update display
  display->display_measurements(measurement);

  delay(100);
}
