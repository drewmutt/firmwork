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
    WiFi.mode(WIFI_STA);
    esp_err_t status = esp_now_init();
    if (status != ESP_OK) {
        throw std::runtime_error(ErrorUtil::getDescriptionFromESPError(status).c_str());
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataReceived));
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


void MeshManager::OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    for(MeshManager *manager : managers)
        manager->dispatchOnDataSent(mac_addr, status);
}

void MeshManager::OnDataReceived(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
    for(MeshManager *manager : managers)
        manager->dispatchOnDataReceived(mac_addr, incomingData, len);
}


void MeshManager::setOnDataSentFunction(void (*aOnDataSentMethod)(MeshManager::MessageReceipt))
{
    this->onDataSentFunction = aOnDataSentMethod;
}

void MeshManager::dispatchOnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    MacAddress macAddress;
    memcpy(macAddress.addressBytes, mac_addr, 6);
    bool wasSuccessful = status == ESP_NOW_SEND_SUCCESS;
    MessageReceipt receipt(macAddress, wasSuccessful);
    if(onDataSentFunction != nullptr)
        onDataSentFunction(receipt);
}


void MeshManager::setOnDataReceivedFunction(void (*aOnDataReceivedFunction)(MessageData))
{
    this->onDataReceivedFunction = aOnDataReceivedFunction;
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

    if(onDataReceivedFunction != nullptr)
        onDataReceivedFunction(messageData);

    free(messageData.message);
}



MeshManager::MessageReceipt::MessageReceipt(MacAddress address, bool success)
{
    this->recipientMacAddress = address;
    this->success = success;
}
