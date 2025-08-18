//
// Created by Andrew Simmons on 8/17/25.
//

#ifndef FIRMWORK_MESHMANAGER_H
#define FIRMWORK_MESHMANAGER_H
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <vector>
#include "MeshNode.h"
#include <esp_now.h>

typedef struct Message
{
    static constexpr int MaxSize = 250;
    int test = 2;
} Message;

class MeshManager
{
    public:
        typedef struct MessageReceipt
        {
            MessageReceipt(MacAddress address, bool b);
            MacAddress recipientMacAddress;
            bool success;
        } MessageReceipt;

        typedef struct MessageData
        {
            MacAddress fromMacAddress;
            MeshNode *fromMeshNode = nullptr;
            Message message;
            int dataLength;
        } MessageData;
        MeshManager();

        static String macAddressToString(MacAddress mac);
        static void changeSelfMacAddress(MacAddress macAddress);
        static wifi_mode_t getSelfWifiMode() { return WiFi.getMode(); }
        static MacAddress getSelfMacAddress();
        static String macAddressToCArrayInitString(MacAddress baseMac);

        void startESPNow();
        void addMeshNode(MeshNode *node);

        template<typename T>
        esp_err_t sendMessage(MeshNode* recipient, const T& msg) {
            return esp_now_send(
                    recipient->getMacAddress().addressBytes,
                    reinterpret_cast<const uint8_t*>(&msg),
                    sizeof(T)
            );
        }


        void setOnDataSentFunction(void (*aOnDataSentFunction)(MessageReceipt));
        void setOnDataReceivedFunction(void (*aOnDataReceivedFunction)(MessageData));

    private:
        static void IRAM_ATTR OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
        static void IRAM_ATTR OnDataReceived(const uint8_t * mac_addr, const uint8_t *incomingData, int len);

        std::vector<MeshNode *> meshNodes;
        void (*onDataSentFunction)(MessageReceipt) = nullptr;
        void (*onDataReceivedFunction)(MessageData) = nullptr;

    protected:
        void dispatchOnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
        void dispatchOnDataReceived(const uint8_t * mac_addr, const uint8_t *incomingData, int len);
        static std::vector<MeshManager *> managers;


};



#endif //FIRMWORK_MESHMANAGER_H
