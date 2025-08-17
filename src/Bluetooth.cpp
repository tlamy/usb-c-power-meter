//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//
#include "Bluetooth.h"

#include "PowerSensor.h"

Bluetooth::Bluetooth(const char* name, const char* serviceId, const char* characteristicId)
    : pServer(nullptr), pCharacteristic(nullptr), deviceConnected(false), oldDeviceConnected(false),
      deviceName(name), serviceUUID(serviceId), characteristicUUID(characteristicId), serverCallbacks(nullptr) {
}

Bluetooth::~Bluetooth() {
    if (serverCallbacks) {
        delete serverCallbacks;
    }
}

bool Bluetooth::begin() {
    Serial.println("Initializing BLE...");

    // Initialize BLE Device
    BLEDevice::init(deviceName);

    // Create BLE Server
    pServer = BLEDevice::createServer();
    if (!pServer) {
        Serial.println("Failed to create BLE server");
        return false;
    }

    // Set up callbacks
    serverCallbacks = new ServerCallbacks(this);
    pServer->setCallbacks(serverCallbacks);

    // Create BLE Service
    BLEService *pService = pServer->createService(serviceUUID);
    if (!pService) {
        Serial.println("Failed to create BLE service");
        return false;
    }

    // Create BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                        characteristicUUID,
                        BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_WRITE |
                        BLECharacteristic::PROPERTY_NOTIFY
                      );

    if (!pCharacteristic) {
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
    pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();

    Serial.println("BLE advertising started. Waiting for client connection...");
    return true;
}

void Bluetooth::handleConnections() {
    // Handle disconnection and restart advertising
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Give the bluetooth stack time to get ready
        pServer->startAdvertising(); // Restart advertising
        Serial.println("BLE advertising restarted");
        oldDeviceConnected = deviceConnected;
    }

    // Handle new connection
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        Serial.println("BLE client connected and ready");
    }
}

bool Bluetooth::isConnected() const {
    return deviceConnected;
}

void Bluetooth::sendData(const PowerMeasurement &measurement) const {
    if (!deviceConnected || !pCharacteristic) {
        return;
    }

    // Create JSON-like string for BLE (more readable than hex)
    char ble_buffer[128];
    snprintf(ble_buffer, sizeof(ble_buffer),
             "{\"current\":%f,\"voltage\":%f,\"timestamp\":%lu}",
             measurement.current, measurement.voltage, millis());
    ble_buffer[sizeof(ble_buffer) - 1] = '\0';

    // Send via BLE characteristic notification
    pCharacteristic->setValue(ble_buffer);
    pCharacteristic->notify();

#ifdef DEBUG_BLE
    Serial.printf("BLE sent: %s\n", ble_buffer);
#endif
}

void Bluetooth::sendRawData(const char* data) {
    if (!deviceConnected || !pCharacteristic) {
        return;
    }

    pCharacteristic->setValue(data);
    pCharacteristic->notify();
}