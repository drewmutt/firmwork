//
// Created by Andrew Simmons on 8/17/25.
//


#include "MeshManager.h"
#include "ErrorUtil.h"
#include "MeshNode.h"


std::vector<MeshManager*> MeshManager::managers;

MeshManager::MeshManager()
{
    MeshManager::managers.push_back(this);
}

String MeshManager::macAddressToString(MacAddress baseMac)
{
    char macBuf[18];
    snprintf(macBuf, sizeof(macBuf),
             "%02x:%02x:%02x:%02x:%02x:%02x",
             (unsigned)baseMac.addressBytes[0], (unsigned)baseMac.addressBytes[1], (unsigned)baseMac.addressBytes[2],
             (unsigned)baseMac.addressBytes[3], (unsigned)baseMac.addressBytes[4], (unsigned)baseMac.addressBytes[5]);

    String macString(macBuf);
    macString.toUpperCase();
    return macString;
}


String MeshManager::macAddressToCArrayInitString(MacAddress baseMac)
{
    char macBuf[37];
    snprintf(macBuf, sizeof(macBuf),
             "{0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x}",
             (unsigned)baseMac.addressBytes[0], (unsigned)baseMac.addressBytes[1], (unsigned)baseMac.addressBytes[2],
             (unsigned)baseMac.addressBytes[3], (unsigned)baseMac.addressBytes[4], (unsigned)baseMac.addressBytes[5]);

    String macString(macBuf);
    return macString;
}

void MeshManager::startESPNow()
{
    WiFiClass::mode(WIFI_STA);
    esp_err_t status = esp_now_init();
    if (status != ESP_OK) {
        throw std::runtime_error(ErrorUtil::getDescriptionFromESPError(status).c_str());
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataReceived);
}

void MeshManager::addMeshNode(MeshNode* node)
{
    esp_now_peer_info_t peerInfo = {};
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    memcpy(peerInfo.peer_addr, node->getMacAddress().addressBytes, 6);
    esp_err_t status = esp_now_add_peer(&peerInfo);

    if(status != ESP_OK)
        throw std::runtime_error(ErrorUtil::getDescriptionFromESPError(status).c_str());

    meshNodes.push_back(node);
}

MacAddress MeshManager::getSelfMacAddress()
{
    if(WiFi.getMode() != WIFI_STA)
        throw std::runtime_error("Wifi needs to be in station mode to get MAC address");

    uint8_t baseMac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);

    if(ret != ESP_OK)
        throw std::runtime_error("Failed to get MAC address");

    MacAddress macAddress;
    memcpy(macAddress.addressBytes, baseMac, 6);
    return macAddress;
}

void MeshManager::changeSelfMacAddress(MacAddress macAddress)
{
    esp_err_t result = esp_wifi_set_mac(WIFI_IF_STA, macAddress.addressBytes);

    if (result != ESP_OK)
    {
        const String &string = ErrorUtil::getDescriptionFromESPError(result);
        throw std::runtime_error(string.c_str());
    }
}


void MeshManager::OnDataSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    if (!tx_info) return;
    const uint8_t *mac_addr = tx_info->des_addr;
    for(MeshManager *manager : managers)
        manager->dispatchOnDataSent(mac_addr, status);
}

void MeshManager::OnDataReceived(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len)
{
    if (!info) return;
    const uint8_t *mac_addr = info->src_addr;
    for(MeshManager *manager : managers)
        manager->dispatchOnDataReceived(mac_addr, incomingData, len);
}


void MeshManager::setOnDataSentFunction(std::function<void(MeshManager::MessageReceipt)> aOnDataSentMethod)
{
    this->onDataSentFunction = std::move(aOnDataSentMethod);
}


void MeshManager::dispatchOnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    MacAddress macAddress;
    memcpy(macAddress.addressBytes, mac_addr, 6);
    bool wasSuccessful = status == ESP_NOW_SEND_SUCCESS;
    MessageReceipt receipt(macAddress, wasSuccessful);
    if(onDataSentFunction)
        onDataSentFunction(receipt);
}

void MeshManager::setOnDataReceivedFunction(std::function<void(MessageData)> aOnDataReceivedFunction)
{
    this->onDataReceivedFunction = std::move(aOnDataReceivedFunction);
}


void MeshManager::dispatchOnDataReceived(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
    MacAddress macAddress;
    memcpy(macAddress.addressBytes, mac_addr, 6);

    MeshManager::MessageData messageData{};

    messageData.fromMacAddress = macAddress;
    messageData.dataLength = len;
    messageData.message = malloc(messageData.dataLength);

    // Kinda ballsy.. dataLength could be a partial message
    memset(messageData.message, 0, sizeof(messageData.dataLength));
    memcpy(messageData.message, incomingData, messageData.dataLength);

    for(MeshNode *node : meshNodes)
        if(memcmp(node->getMacAddress().addressBytes, mac_addr, 6) == 0)
            messageData.fromMeshNode = node;

    if(onDataReceivedFunction)
        onDataReceivedFunction(messageData);

    free(messageData.message);
}

/*
 * ArduinoOTAClass::THandlerFunction start = [](unsigned int progress, unsigned int total) {
            lcd.setCursor(0, 1);
            lcd.printf("Progress: %u%%", (progress / (total / 100)));
            lcd.display();
    }
 */
bool MeshManager::startListeningForOTA(unsigned long syncWaitTimeMSec, ArduinoOTAClass::THandlerFunction onStart, ArduinoOTAClass::THandlerFunction_Progress onProgress, ArduinoOTAClass::THandlerFunction_Error onError, ArduinoOTAClass::THandlerFunction onEnd)
{
    if(WiFi.status() != WL_CONNECTED)
        return false;

    ArduinoOTAClass &aClass = ArduinoOTA;
    if(onStart != nullptr)
        aClass.onStart(onStart);
    if(onProgress != nullptr)
        aClass.onProgress(onProgress);
    if(onError != nullptr)
        aClass.onError(onError);
    if(onEnd != nullptr)
        aClass.onEnd(onEnd);

    ArduinoOTA.begin();

    // HANG. Waiting...
    if(syncWaitTimeMSec > 0)
    {
        unsigned long startTime = millis();

        while(WiFi.isConnected())
        {
            Serial.println("OTA waiting");
            ArduinoOTA.handle();
            delay(1000);
            if(millis() - startTime > syncWaitTimeMSec)
                break;
        }
        Serial.println("OTA timeout");
    }

    return false;
}

bool MeshManager::connectToWifi(String ssid, String password, int attempts, int delayBetweenAttempts)
{
    WiFi.begin(ssid, password);  // Connect to WiFi - defaults to WiFi Station mode
    bool success = false;
    for(int i = 0; i <= attempts; i++)
    {
        if(WiFi.status() == WL_CONNECTED)
        {
            success = true;
            break;
        }
        delay(delayBetweenAttempts);
    }
    return success;
}

bool MeshManager::disconnectFromWifi()
{
    bool success = WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return success;
}

tm MeshManager::getTimeFromNTPServer()
{
    if(!WiFi.isConnected())
        return {};

    const char* ntpServer = "pool.ntp.org";
    const long  gmtOffset_sec = -21600; //-18000;
    const long  daylightOffset_sec = 3600;

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo{};
    struct tm updatedTimeinfo{};

    if (getLocalTime(&timeinfo)) {
        updatedTimeinfo.tm_sec = timeinfo.tm_sec;
        updatedTimeinfo.tm_min = timeinfo.tm_min;
        updatedTimeinfo.tm_hour = timeinfo.tm_hour;
        updatedTimeinfo.tm_wday = timeinfo.tm_wday + 1;
        updatedTimeinfo.tm_mon = timeinfo.tm_mon + 1;
        updatedTimeinfo.tm_year = timeinfo.tm_year - 100;
        return updatedTimeinfo;
    }
    return {};
}

MeshManager::MessageReceipt::MessageReceipt(MacAddress address, bool success)
{
    this->recipientMacAddress = address;
    this->success = success;
}

