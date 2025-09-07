//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//
#include "Bluetooth.h"
#include "PowerSensor.h"
#include <BLE2902.h>
#include <BLEDevice.h>

Bluetooth::Bluetooth(const char *name, const char *serviceId,
                     const char *characteristicId)
    : pServer(nullptr), pCharacteristic(nullptr), deviceConnected(false),
      oldDeviceConnected(false), serviceUUID(serviceId),
      characteristicUUID(characteristicId), deviceName(name),
      serverCallbacks(nullptr) {}

Bluetooth::~Bluetooth() { delete serverCallbacks; }

bool Bluetooth::begin() {
  // Initialize BLE Device
  BLEDevice::init(deviceName);

  // Create BLE Server
  pServer = BLEDevice::createServer();
  if (pServer == nullptr) {
    Serial.println("Failed to create BLE server");
    return false;
  }

  // Set up callbacks
  serverCallbacks = new ServerCallbacks(this);
  pServer->setCallbacks(serverCallbacks);

  // Create BLE Service
  BLEService *pService = pServer->createService(serviceUUID);
  if (pService == nullptr) {
    Serial.println("Failed to create BLE service");
    return false;
  }

  // Create BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
      characteristicUUID, BLECharacteristic::PROPERTY_READ |
                              BLECharacteristic::PROPERTY_WRITE |
                              BLECharacteristic::PROPERTY_NOTIFY);

  if (pCharacteristic == nullptr) {
    Serial.println("Failed to create BLE characteristic");
    return false;
  }

  // Add BLE2902 descriptor for notifications
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(serviceUUID);
  pAdvertising->setScanResponse(false);
  // set the value to 0x00 to not advertise this parameter
  pAdvertising->setMinPreferred( 0x0);
  BLEDevice::startAdvertising();

  Serial.println("BLE advertising started. Waiting for client connection...");
  return true;
}

void Bluetooth::handleConnections() {
  // Handle disconnection and restart advertising
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                  // Give the bluetooth stack time to get ready
    pServer->startAdvertising(); // Restart advertising
    // Serial.println("BLE advertising restarted");
    oldDeviceConnected = deviceConnected;
  }

  // Handle a new connection
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    // Serial.println("BLE client connected and ready");
  }
}

bool Bluetooth::isConnected() const { return deviceConnected; }

void Bluetooth::sendData(const PowerMeasurement &measurement) const {
  if (!deviceConnected || pCharacteristic == nullptr) {
    return;
  }

  char ble_buffer[128];
  snprintf(ble_buffer, sizeof(ble_buffer),
           "{\"current\":%f,\"voltage\":%f,\"power\":%f,\"charge\":%f,"
           "\"timestamp\":%lu}",
           measurement.current, measurement.voltage, measurement.power,
           measurement.charge, millis());
  ble_buffer[sizeof(ble_buffer) - 1] = '\0';

#ifdef DEBUG_BLE
  Serial.printf("BLE sent: '%s'\n", ble_buffer);
#endif
  // Send via BLE characteristic notification
  pCharacteristic->setValue(ble_buffer);
  pCharacteristic->notify();

#ifdef DEBUG_BLE
  Serial.printf("BLE sent: %s\n", ble_buffer);
#endif
}
