//
// Created by Andrew Simmons on 2/25/26.
//

#include <BLEUartManager.h>
#include <BLEDescriptor.h>
#include <BLE2904.h>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#if FW_BT_BLEUART_DEBUG
#define BLEUART_DEBUG_PRINTLN(msg) Serial.println(String("[BLEUartManager] ") + String(msg))
#define BLEUART_DEBUG_PRINTF(fmt, ...) Serial.printf("[BLEUartManager] " fmt "\n", ##__VA_ARGS__)
#else
#define BLEUART_DEBUG_PRINTLN(msg) do {} while (0)
#define BLEUART_DEBUG_PRINTF(fmt, ...) do {} while (0)
#endif

class BLEUartManager::ServerCallbacks : public BLEServerCallbacks
{
public:
    explicit ServerCallbacks(BLEUartManager* aManager)
        : manager(aManager)
    {
    }

    void onConnect(BLEServer* pServer) override
    {
        (void)pServer;
        this->manager->onBleConnected();
    }

    void onDisconnect(BLEServer* pServer) override
    {
        this->manager->onBleDisconnected(pServer);
    }

private:
    BLEUartManager* manager;
};

class BLEUartManager::RxCallbacks : public BLECharacteristicCallbacks
{
public:
    explicit RxCallbacks(BLEUartManager* aManager)
        : manager(aManager)
    {
    }

    void onWrite(BLECharacteristic* pCharacteristic) override
    {
        this->manager->onRxWrite(pCharacteristic);
    }

private:
    BLEUartManager* manager;
};

class BLEUartManager::SettingCallbacks : public BLECharacteristicCallbacks
{
public:
    SettingCallbacks(BLEUartManager* aManager, const String& aKey)
        : manager(aManager),
          key(aKey)
    {
    }

    void onWrite(BLECharacteristic* pCharacteristic) override
    {
        this->manager->onSettingCharacteristicWrite(this->key, pCharacteristic);
    }

    void onRead(BLECharacteristic* pCharacteristic) override
    {
        this->manager->onSettingCharacteristicRead(this->key, pCharacteristic);
    }

private:
    BLEUartManager* manager;
    String key;
};

class NoopDescriptorCallbacks : public BLEDescriptorCallbacks
{
public:
    void onRead(BLEDescriptor* pDescriptor) override
    {
        (void)pDescriptor;
    }

    void onWrite(BLEDescriptor* pDescriptor) override
    {
        (void)pDescriptor;
    }
};

static NoopDescriptorCallbacks gNoopDescriptorCallbacks;

BLEUartManager::BLEUartManager(const String& aDeviceName, Settings* aSettings, const UuidConfig& aUuidConfig)
    : deviceName(aDeviceName),
      uuidConfig(aUuidConfig),
      settings(aSettings),
      serialPipePrint(this)
{
}

BLEUartManager::BLEUartManager(const String& aDeviceName, Settings* aSettings)
    : BLEUartManager(aDeviceName, aSettings, UuidConfig())
{
}

size_t BLEUartManager::SerialPipePrint::write(uint8_t c)
{
    if (this->echoTarget != nullptr)
        this->echoTarget->write(c);

    this->pendingBuffer += static_cast<char>(c);

    if (c == '\n' || this->pendingBuffer.length() >= MaxBufferedPayload)
    {
        if (this->owner != nullptr)
            this->owner->sendResponse(this->pendingBuffer);

        this->pendingBuffer = "";
    }

    return 1;
}

void BLEUartManager::SerialPipePrint::flushPending()
{
    if (this->pendingBuffer.length() == 0)
        return;

    if (this->owner != nullptr)
        this->owner->sendResponse(this->pendingBuffer);

    this->pendingBuffer = "";
}

bool BLEUartManager::begin()
{
    BLEUART_DEBUG_PRINTF("begin() device=%s", this->deviceName.c_str());
    if (this->settings == nullptr)
    {
        BLEUART_DEBUG_PRINTLN("begin() failed: settings is null");
        return false;
    }

    if (!this->settings->begin())
    {
        BLEUART_DEBUG_PRINTLN("begin() failed: settings->begin() returned false");
        return false;
    }

    this->serialPipePrint.setLocalEcho(&Serial);

    BLEDevice::init(this->deviceName.c_str());
    BLEUART_DEBUG_PRINTLN("BLEDevice initialized");

    this->server = BLEDevice::createServer();
    this->server->setCallbacks(new ServerCallbacks(this));

    BLEService* uartService = this->server->createService(this->uuidConfig.serviceUuid);

    this->txCharacteristic = uartService->createCharacteristic(
        this->uuidConfig.txCharacteristicUuid,
        BLECharacteristic::PROPERTY_NOTIFY
    );

    this->rxCharacteristic = uartService->createCharacteristic(
        this->uuidConfig.rxCharacteristicUuid,
        BLECharacteristic::PROPERTY_WRITE
    );
    this->rxCharacteristic->setCallbacks(new RxCallbacks(this));

    this->createSettingsCharacteristics(this->server);

    uartService->start();
    BLEUART_DEBUG_PRINTLN("BLE UART service started");

    if (this->settingsService != nullptr)
    {
        this->settingsService->start();
        BLEUART_DEBUG_PRINTLN("BLE settings service started");
    }

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(this->uuidConfig.serviceUuid);

    if (this->settingsService != nullptr)
        advertising->addServiceUUID(this->uuidConfig.settingsServiceUuid);

    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    BLEDevice::startAdvertising();

    BLEUART_DEBUG_PRINTF(
        "Advertising started uartService=%s settingsService=%s rx=%s tx=%s",
        this->uuidConfig.serviceUuid,
        this->uuidConfig.settingsServiceUuid,
        this->uuidConfig.rxCharacteristicUuid,
        this->uuidConfig.txCharacteristicUuid
    );

    this->installSettingsListener();
    BLEUART_DEBUG_PRINTLN("begin() complete");
    return true;
}

void BLEUartManager::end()
{
    BLEUART_DEBUG_PRINTLN("end()");
    this->serialPipePrint.flushPending();

    if (this->settings != nullptr)
        this->settings->end();
}

bool BLEUartManager::isConnected() const
{
    return this->deviceConnected;
}

bool BLEUartManager::sendResponse(const String& message)
{
    if (!this->deviceConnected || this->txCharacteristic == nullptr)
    {
        BLEUART_DEBUG_PRINTLN("sendResponse skipped: no connection or txCharacteristic");
        return false;
    }

    BLEUART_DEBUG_PRINTLN("UART TX: " + message);
    this->txCharacteristic->setValue(reinterpret_cast<const uint8_t*>(message.c_str()), message.length());
    this->txCharacteristic->notify();
    return true;
}

void BLEUartManager::onBleConnected()
{
    this->deviceConnected = true;
    BLEUART_DEBUG_PRINTLN("BLE client connected");
}

void BLEUartManager::onBleDisconnected(BLEServer* pServer)
{
    this->deviceConnected = false;
    BLEUART_DEBUG_PRINTLN("BLE client disconnected, restarting advertising");
    pServer->startAdvertising();
}

void BLEUartManager::onRxWrite(BLECharacteristic* pCharacteristic)
{
    const String payload = pCharacteristic->getValue();
    BLEUART_DEBUG_PRINTLN("UART RX payload: " + payload);

    if (this->uartRxCallback != nullptr)
        this->uartRxCallback(payload);
}

void BLEUartManager::onSettingCharacteristicRead(const String& key, BLECharacteristic* pCharacteristic)
{
    (void)pCharacteristic;
    this->updateSettingCharacteristicValue(key, false);

    const SettingField* field = this->findSettingField(key);
    if (field == nullptr)
    {
        BLEUART_DEBUG_PRINTLN("GATT READ key=" + key + " field-missing");
        return;
    }

    String currentValue;
    this->settingFieldValueToLogString(*field, currentValue);
    BLEUART_DEBUG_PRINTLN("GATT READ key=" + key + " value=" + currentValue);
}

void BLEUartManager::onSettingCharacteristicWrite(const String& key, BLECharacteristic* pCharacteristic)
{
    const uint8_t* rawData = pCharacteristic->getData();
    const size_t rawLength = pCharacteristic->getLength();
    BLEUART_DEBUG_PRINTLN(
        "GATT WRITE key=" + key +
        " len=" + String(static_cast<unsigned int>(rawLength)) +
        " hex=" + this->payloadToHex(rawData, rawLength)
    );

    const SettingField* field = this->findSettingField(key);
    if (field == nullptr)
    {
        BLEUART_DEBUG_PRINTLN("GATT WRITE ignored: unknown key");
        return;
    }

    if (!this->applySettingFieldValueFromCharacteristic(*field, rawData, rawLength))
    {
        BLEUART_DEBUG_PRINTLN("GATT WRITE parse failed key=" + key);
        this->updateSettingCharacteristicValue(key, false);
        return;
    }

    String parsedValue;
    this->settingFieldValueToLogString(*field, parsedValue);
    BLEUART_DEBUG_PRINTLN("GATT WRITE applied key=" + key + " parsed=" + parsedValue + " (persist=true)");

    this->settings->notifyChanged(key, true);
}

void BLEUartManager::installSettingsListener()
{
    if (this->settings == nullptr || this->settingsListenerInstalled)
    {
        BLEUART_DEBUG_PRINTLN("installSettingsListener skipped");
        return;
    }

    this->settings->addOnChangeListener([this](const String& key)
    {
        BLEUART_DEBUG_PRINTLN("settings changed key=" + key + " autoNotify=" + String(this->autoNotifyOnSettingChanged ? "true" : "false"));
        this->updateSettingCharacteristicValue(key, this->autoNotifyOnSettingChanged);
    });

    this->settingsListenerInstalled = true;
    BLEUART_DEBUG_PRINTLN("settings listener installed");
}

void BLEUartManager::createSettingsCharacteristics(BLEServer* pServer)
{
    if (this->settings == nullptr || pServer == nullptr)
        return;

    this->settingsService = pServer->createService(this->uuidConfig.settingsServiceUuid);
    const std::vector<SettingField>& fields = this->settings->getFields();

    for (const SettingField& field : fields)
    {
        const String key = field.name;
        const String uuid = this->createSettingCharacteristicUuid(key);
        const String displayName =
            (field.description != nullptr && field.description[0] != '\0')
            ? String(field.description)
            : key;

        BLEUUID settingUuid(uuid.c_str());
        BLECharacteristic* characteristic = this->settingsService->createCharacteristic(
            settingUuid,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
        );

        BLE2901* userDescription = new BLE2901();
        userDescription->setDescription(displayName);
        userDescription->setCallbacks(&gNoopDescriptorCallbacks);
        characteristic->addDescriptor(userDescription);

        BLE2904* presentationFormat = new BLE2904();
        switch (field.type)
        {
            case SettingType::INT:
                presentationFormat->setFormat(BLE2904::FORMAT_SINT32);
                break;
            case SettingType::FLOAT:
                presentationFormat->setFormat(BLE2904::FORMAT_FLOAT32);
                break;
            case SettingType::BOOL:
                presentationFormat->setFormat(BLE2904::FORMAT_BOOLEAN);
                break;
            case SettingType::STRING:
                presentationFormat->setFormat(BLE2904::FORMAT_UTF8);
                break;
        }
        presentationFormat->setCallbacks(&gNoopDescriptorCallbacks);
        characteristic->addDescriptor(presentationFormat);

        characteristic->setCallbacks(new SettingCallbacks(this, key));

        SettingCharacteristicBinding binding{};
        binding.key = key;
        binding.uuid = uuid;
        binding.characteristic = characteristic;
        this->settingCharacteristics.push_back(binding);

        this->updateSettingCharacteristicValue(key, false);

        BLEUART_DEBUG_PRINTLN(
            "Created setting characteristic key=" + key +
            " uuid=" + uuid +
            " name=" + displayName
        );
    }
}

bool BLEUartManager::updateSettingCharacteristicValue(const String& key, bool notify)
{
    BLECharacteristic* characteristic = this->findSettingCharacteristic(key);
    const SettingField* field = this->findSettingField(key);

    if (characteristic == nullptr || field == nullptr)
        return false;

    if (!this->settingFieldValueToCharacteristic(*field, characteristic))
        return false;

    String value;
    this->settingFieldValueToLogString(*field, value);
    BLEUART_DEBUG_PRINTLN("updateSettingCharacteristicValue key=" + key + " value=" + value + (notify ? " notify=true" : " notify=false"));

    if (notify && this->deviceConnected)
    {
        characteristic->notify();
        BLEUART_DEBUG_PRINTLN("updateSettingCharacteristicValue notified key=" + key);
    }

    return true;
}

BLECharacteristic* BLEUartManager::findSettingCharacteristic(const String& key) const
{
    for (const SettingCharacteristicBinding& binding : this->settingCharacteristics)
    {
        if (binding.key == key)
            return binding.characteristic;
    }

    return nullptr;
}

String BLEUartManager::findSettingCharacteristicUuid(const String& key) const
{
    for (const SettingCharacteristicBinding& binding : this->settingCharacteristics)
    {
        if (binding.key == key)
            return binding.uuid;
    }

    return this->createSettingCharacteristicUuid(key);
}

String BLEUartManager::createSettingCharacteristicUuid(const String& key) const
{
    uint32_t hash = 2166136261u; // FNV-1a
    for (size_t i = 0; i < key.length(); i++)
    {
        hash ^= static_cast<uint8_t>(key[i]);
        hash *= 16777619u;
    }

    char uuid[37];
    snprintf(uuid, sizeof(uuid), "%08lX-B5A3-F393-E0A9-E50E24DCCA9E", static_cast<unsigned long>(hash));
    return String(uuid);
}

const SettingField* BLEUartManager::findSettingField(const String& key) const
{
    if (this->settings == nullptr)
        return nullptr;

    const std::vector<SettingField>& fields = this->settings->getFields();
    for (const SettingField& field : fields)
    {
        if (key.equals(field.name))
            return &field;
    }

    return nullptr;
}

bool BLEUartManager::settingFieldValueToLogString(const SettingField& field, String& outValue) const
{
    outValue = "";

    switch (field.type)
    {
        case SettingType::INT:
            outValue = String(*static_cast<int*>(field.ptr));
            return true;
        case SettingType::FLOAT:
            outValue = String(*static_cast<float*>(field.ptr), 6);
            return true;
        case SettingType::BOOL:
            outValue = *static_cast<bool*>(field.ptr) ? "true" : "false";
            return true;
        case SettingType::STRING:
            outValue = *static_cast<String*>(field.ptr);
            return true;
    }

    return false;
}

bool BLEUartManager::settingFieldValueToCharacteristic(const SettingField& field, BLECharacteristic* characteristic) const
{
    if (characteristic == nullptr)
        return false;

    switch (field.type)
    {
        case SettingType::INT:
        {
            const int32_t value = static_cast<int32_t>(*static_cast<int*>(field.ptr));
            characteristic->setValue(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
            return true;
        }

        case SettingType::FLOAT:
        {
            const float value = *static_cast<float*>(field.ptr);
            characteristic->setValue(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
            return true;
        }

        case SettingType::BOOL:
        {
            const uint8_t value = *static_cast<bool*>(field.ptr) ? 1 : 0;
            characteristic->setValue(&value, sizeof(value));
            return true;
        }

        case SettingType::STRING:
        {
            const String& value = *static_cast<String*>(field.ptr);
            characteristic->setValue(reinterpret_cast<const uint8_t*>(value.c_str()), value.length());
            return true;
        }
    }

    return false;
}

bool BLEUartManager::applySettingFieldValueFromCharacteristic(const SettingField& field, const uint8_t* data, size_t length)
{
    if (data == nullptr && length > 0)
        return false;

    BLEUART_DEBUG_PRINTLN(
        "applySettingFieldValueFromCharacteristic key=" + String(field.name) +
        " len=" + String(static_cast<unsigned int>(length)) +
        " hex=" + this->payloadToHex(data, length)
    );

    switch (field.type)
    {
        case SettingType::INT:
        {
            if (length == sizeof(int32_t))
            {
                int32_t parsed = 0;
                memcpy(&parsed, data, sizeof(parsed));
                *static_cast<int*>(field.ptr) = static_cast<int>(parsed);
                return true;
            }

            String text = (data != nullptr && length > 0)
                ? String(reinterpret_cast<const char*>(data), length)
                : String("");
            text.trim();
            BLEUART_DEBUG_PRINTLN("INT! len " + String(length));
            BLEUART_DEBUG_PRINTLN("INT! T " + text);
            for (size_t i = 0; i < length; ++i)
            {
                BLEUART_DEBUG_PRINTLN("INT " + String(static_cast<unsigned int>(i)) + "! " + String(data[i]));
            }

            bool looksTextual = length > 0;
            for (size_t i = 0; i < length; ++i)
            {
                const uint8_t c = data[i];
                const bool isSign = (i == 0U) && (c == '+' || c == '-');
                const bool isDigit = c >= '0' && c <= '9';
                const bool isWs = c == ' ' || c == '\t' || c == '\r' || c == '\n';
                if (!(isSign || isDigit || isWs))
                {
                    looksTextual = false;
                    break;
                }
            }

            if (looksTextual)
            {
                const char* raw = text.c_str();
                char* endPtr = nullptr;
                const long parsed = strtol(raw, &endPtr, 10);
                BLEUART_DEBUG_PRINTLN("INT! R " + String(raw));
                BLEUART_DEBUG_PRINTLN("INT! P " + String(parsed));
                if (!(endPtr == raw || (endPtr != nullptr && *endPtr != '\0')))
                {
                    *static_cast<int*>(field.ptr) = static_cast<int>(parsed);
                    return true;
                }
            }

            // Accept compact little-endian binary integer payloads (1..4 bytes),
            // e.g. a single byte 0x06 for value 6.
            if (length > 0 && length < sizeof(int32_t))
            {
                uint32_t raw = 0U;
                memcpy(&raw, data, length);

                if ((data[length - 1U] & 0x80U) != 0U)
                {
                    const uint32_t signMask = ~((1UL << (length * 8U)) - 1UL);
                    raw |= signMask;
                }

                const int32_t parsed = static_cast<int32_t>(raw);
                BLEUART_DEBUG_PRINTLN("INT! binary parsed " + String(parsed));
                *static_cast<int*>(field.ptr) = static_cast<int>(parsed);
                return true;
            }

            return false;
        }

        case SettingType::FLOAT:
        {
            if (length == sizeof(float))
            {
                float parsed = 0.0f;
                memcpy(&parsed, data, sizeof(parsed));
                *static_cast<float*>(field.ptr) = parsed;
                return true;
            }

            String text = (data != nullptr && length > 0)
                ? String(reinterpret_cast<const char*>(data), length)
                : String("");
            text.trim();
            const char* raw = text.c_str();
            char* endPtr = nullptr;
            const float parsed = strtof(raw, &endPtr);
            if (endPtr == raw || (endPtr != nullptr && *endPtr != '\0'))
                return false;
            *static_cast<float*>(field.ptr) = parsed;
            return true;
        }

        case SettingType::BOOL:
        {
            if (length == sizeof(uint8_t))
            {
                *static_cast<bool*>(field.ptr) = data[0] != 0;
                return true;
            }

            String text = (data != nullptr && length > 0)
                ? String(reinterpret_cast<const char*>(data), length)
                : String("");
            text.trim();
            if (text.equalsIgnoreCase("true") || text.equalsIgnoreCase("1") || text.equalsIgnoreCase("on"))
            {
                *static_cast<bool*>(field.ptr) = true;
                return true;
            }

            if (text.equalsIgnoreCase("false") || text.equalsIgnoreCase("0") || text.equalsIgnoreCase("off"))
            {
                *static_cast<bool*>(field.ptr) = false;
                return true;
            }

            return false;
        }

        case SettingType::STRING:
        {
            *static_cast<String*>(field.ptr) = (data != nullptr && length > 0)
                ? String(reinterpret_cast<const char*>(data), length)
                : String("");
            return true;
        }
    }

    return false;
}

String BLEUartManager::payloadToHex(const uint8_t* data, size_t length) const
{
    if (data == nullptr || length == 0)
        return "";

    static constexpr size_t MaxBytesToPrint = 24;
    const size_t bytesToPrint = (length > MaxBytesToPrint) ? MaxBytesToPrint : length;

    String hex;
    hex.reserve(bytesToPrint * 3 + 8);
    char byteBuffer[4];

    for (size_t i = 0; i < bytesToPrint; i++)
    {
        if (i > 0)
            hex += ' ';
        snprintf(byteBuffer, sizeof(byteBuffer), "%02X", static_cast<unsigned int>(data[i]));
        hex += byteBuffer;
    }

    if (bytesToPrint < length)
        hex += " ...";

    return hex;
}
