//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#ifndef USB_POWER_FIRMWARE_ESP8266_BLUETOOTH_H
#define USB_POWER_FIRMWARE_ESP8266_BLUETOOTH_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include "PowerSensor.h"

class Bluetooth {
private:
    BLEServer *pServer;
    BLECharacteristic *pCharacteristic;
    bool deviceConnected;
    bool oldDeviceConnected;

    const char *serviceUUID;
    const char *characteristicUUID;
    const char *deviceName;

    // Internal callback class
    class ServerCallbacks final : public BLEServerCallbacks {
    private:
        Bluetooth *bluetooth;

    public:
        ServerCallbacks(Bluetooth *bt) : bluetooth(bt) {
        }

        void onConnect(BLEServer *bleServer) override {
            bluetooth->deviceConnected = true;
            Serial.println("BLE Client Connected");
        }

        void onDisconnect(BLEServer *bleServer) override {
            bluetooth->deviceConnected = false;
            Serial.println("BLE Client Disconnected");
        }
    };

    ServerCallbacks *serverCallbacks;

public:
    Bluetooth(const char *name, const char *serviceId, const char *characteristicId);

    ~Bluetooth();

    bool begin();

    void handleConnections();

    bool isConnected() const;

    void sendData(const PowerMeasurement &measurement) const;

    void sendRawData(const char *data);
};

#endif //USB_POWER_FIRMWARE_ESP8266_BLUETOOTH_H
