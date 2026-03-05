//
// Created by Andrew Simmons on 2/25/26.
//

#ifndef FIRMWORK_BLEUARTMANAGER_H
#define FIRMWORK_BLEUARTMANAGER_H

#include <Arduino.h>
#include <BLE2901.h>
#include <BLECharacteristic.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <Settings.h>
#include <functional>
#include <vector>

// Set to 0 before including this header to disable BLE UART debug logs.
#ifndef FW_BT_BLEUART_DEBUG
#define FW_BT_BLEUART_DEBUG 1
#endif

class BLEUartManager
{
public:
    static constexpr const char* DefaultServiceUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    static constexpr const char* DefaultSettingsServiceUuid = "6E400010-B5A3-F393-E0A9-E50E24DCCA9E";
    static constexpr const char* DefaultRxCharacteristicUuid = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    static constexpr const char* DefaultTxCharacteristicUuid = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

    typedef struct UuidConfig
    {
        UuidConfig(
            const char* aServiceUuid = DefaultServiceUuid,
            const char* aSettingsServiceUuid = DefaultSettingsServiceUuid,
            const char* aRxCharacteristicUuid = DefaultRxCharacteristicUuid,
            const char* aTxCharacteristicUuid = DefaultTxCharacteristicUuid
        )
            : serviceUuid(aServiceUuid),
              settingsServiceUuid(aSettingsServiceUuid),
              rxCharacteristicUuid(aRxCharacteristicUuid),
              txCharacteristicUuid(aTxCharacteristicUuid)
        {
        }

        const char* serviceUuid;
        const char* settingsServiceUuid;
        const char* rxCharacteristicUuid;
        const char* txCharacteristicUuid;
    } UuidConfig;

    BLEUartManager(const String& aDeviceName, Settings* aSettings);
    BLEUartManager(const String& aDeviceName, Settings* aSettings, const UuidConfig& aUuidConfig);
    virtual ~BLEUartManager() = default;

    typedef std::function<void(const String& payload)> UartRxCallback;

    class SerialPipePrint : public Print
    {
    public:
        explicit SerialPipePrint(BLEUartManager* aOwner)
            : owner(aOwner)
        {
        }

        size_t write(uint8_t c) override;
        void setLocalEcho(Print* aEchoTarget) { this->echoTarget = aEchoTarget; }
        void flushPending();

    private:
        BLEUartManager* owner = nullptr;
        Print* echoTarget = nullptr;
        String pendingBuffer{};
        static constexpr size_t MaxBufferedPayload = 180;
    };

    bool begin();
    void end();

    bool isConnected() const;
    bool sendResponse(const String& message);
    bool sendUart(const String& message) { return this->sendResponse(message); }

    void setUartRxCallback(UartRxCallback callback) { this->uartRxCallback = callback; }
    Print& getSerialPipePrint() { return this->serialPipePrint; }
    void setSerialPipeEcho(Print* echoTarget) { this->serialPipePrint.setLocalEcho(echoTarget); }
    void flushSerialPipe() { this->serialPipePrint.flushPending(); }

    Settings* getSettings() const { return this->settings; }
    void setAutoNotifyOnSettingChanged(bool enabled) { this->autoNotifyOnSettingChanged = enabled; }

private:
    class ServerCallbacks;
    class RxCallbacks;
    class SettingCallbacks;

    typedef struct SettingCharacteristicBinding
    {
        String key;
        String uuid;
        BLECharacteristic* characteristic = nullptr;
    } SettingCharacteristicBinding;

    void onBleConnected();
    void onBleDisconnected(BLEServer* pServer);
    void onRxWrite(BLECharacteristic* pCharacteristic);
    void onSettingCharacteristicRead(const String& key, BLECharacteristic* pCharacteristic);
    void onSettingCharacteristicWrite(const String& key, BLECharacteristic* pCharacteristic);
    void installSettingsListener();
    void createSettingsCharacteristics(BLEServer* pServer);
    bool updateSettingCharacteristicValue(const String& key, bool notify);
    BLECharacteristic* findSettingCharacteristic(const String& key) const;
    String findSettingCharacteristicUuid(const String& key) const;
    String createSettingCharacteristicUuid(const String& key) const;
    const SettingField* findSettingField(const String& key) const;
    bool settingFieldValueToLogString(const SettingField& field, String& outValue) const;
    bool settingFieldValueToCharacteristic(const SettingField& field, BLECharacteristic* characteristic) const;
    bool applySettingFieldValueFromCharacteristic(const SettingField& field, const uint8_t* data, size_t length);
    String payloadToHex(const uint8_t* data, size_t length) const;

    String deviceName;
    UuidConfig uuidConfig;
    Settings* settings = nullptr;
    BLEServer* server = nullptr;
    BLEService* settingsService = nullptr;
    BLECharacteristic* txCharacteristic = nullptr;
    BLECharacteristic* rxCharacteristic = nullptr;
    bool deviceConnected = false;
    bool autoNotifyOnSettingChanged = true;
    bool settingsListenerInstalled = false;
    UartRxCallback uartRxCallback = nullptr;
    SerialPipePrint serialPipePrint;
    std::vector<SettingCharacteristicBinding> settingCharacteristics;
};

#endif //FIRMWORK_BLEUARTMANAGER_H
