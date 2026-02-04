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
#include <ArduinoOTA.h>
#include "../../../Firmwork-Common/include/Application.h"

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,0,0)
#define OLD_VERSION_ESP
#endif

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
            int dataLength;
            void *message;
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

        void setOnDataSentFunction(std::function<void(MessageReceipt)> aOnDataSentFunction);
        void setOnDataReceivedFunction(std::function<void(MessageData)> aOnDataReceivedFunction);

        static bool connectToWifi(String ssid, String password, int attempts = 5, int delayBetweenAttemptsMSec = 500);
        bool startListeningForOTA(unsigned long syncWaitTimeMSec = 0, ArduinoOTAClass::THandlerFunction onStart = nullptr, ArduinoOTAClass::THandlerFunction_Progress onProgress = nullptr, ArduinoOTAClass::THandlerFunction_Error onError = nullptr, ArduinoOTAClass::THandlerFunction onEnd = nullptr);

        static tm getTimeFromNTPServer();
        static bool disconnectFromWifi();
        Timer *otaTimeoutTimer;
    private:
#ifdef OLD_VERSION_ESP
    static void IRAM_ATTR OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void IRAM_ATTR OnDataReceived(const uint8_t *mac_addr, const uint8_t *data, int data_len);
#else
    static void IRAM_ATTR OnDataSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status);
    static void IRAM_ATTR OnDataReceived(const esp_now_recv_info_t * info, const uint8_t *incomingData, int len);
#endif


        std::vector<MeshNode *> meshNodes;
        std::function<void(MessageReceipt)> onDataSentFunction;
        std::function<void(MessageData)> onDataReceivedFunction;

    protected:
        void dispatchOnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
        void dispatchOnDataReceived(const uint8_t * mac_addr, const uint8_t *incomingData, int len);
        static std::vector<MeshManager *> managers;

};



#endif //FIRMWORK_MESHMANAGER_H
