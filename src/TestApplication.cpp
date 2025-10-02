//
// Created by Andrew Simmons on 8/18/25.
//

#include <HardwareSerial.h>
#include "TestApplication.h"
#include "mesh/MeshNode.h"
#include "ErrorUtil.h"
#include "Wifi.h"


#define MASTER false
MeshNode MasterMeshNode({0x44, 0x1d, 0x64, 0xf8, 0x01, 0x1c});

bool TryingOTA = false;
typedef struct MyMessage: public Message
{
    char propertyName[30] = {};
    double propertyValue = 0;
    byte unitType;
} SensorMessage;

void TestApplication::setup()
{
    Serial.begin(9600);

    Bounds bounds({50,50}, {500, 1000});

    auto top_left_of = bounds.topLeftOf({100, 200}, BoundsAnchor::BOTTOM_RIGHT);
    Serial.println("Top left of: " + String(top_left_of.x) + ", " + String(top_left_of.y));

    delay(2000);
    Serial.println("Role: " + String(MASTER==true?"MASTER":"SLAVE"));;
    WiFi.setHostname("truckESP32");

    Serial.println("Connecting to MunchausenByProxy...");
    bool success = MeshManager::connectToWifi("MunchausenByProxy", "apples2apples", 3, 1000);
    TryingOTA = true;


    Serial.println("Success?" + String(success));;

    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if(success)
    {
        Serial.println("Starting OTA ");
        meshManager->startListeningForOTA(0, []
                                          {
                                              Serial.println("OTA Started");;
                                          }, [](unsigned int progress, unsigned int total)
                                          {
                                              Serial.println("Progress: " + String(progress) + " / " + String(total));;
                                          });
    }

    Serial.println("Dis from wifi...");
//    MeshManager::disconnectFromWifi();
    Serial.println("dunet from wifi...");;
    meshManager->startESPNow();
    Serial.println("started ESPNow");

    if (MASTER)
    {
        meshManager->setOnDataReceivedFunction(&TestApplication::onGotData);
    } else
    {
        meshManager->addMeshNode(&MasterMeshNode);
        meshManager->setOnDataSentFunction(&TestApplication::onDataSent);;
    }

    const MacAddress &macAddress = MeshManager::getSelfMacAddress();
    Serial.println(MeshManager::macAddressToString(macAddress));
    const String &string = MeshManager::macAddressToCArrayInitString(macAddress);
    Serial.println("MAC:" + string);

}

void TestApplication::loop()
{
    ArduinoOTA.handle();
    if(!WiFi.isConnected())
    {
        if (!MASTER)
        {
            MyMessage msg;
            msg.propertyName[0] = 'X';
            msg.unitType = 1;
            msg.propertyValue = 100;

            esp_err_t i = meshManager->sendMessage(&MasterMeshNode, msg);
            Serial.println("sent message.. " + ErrorUtil::getDescriptionFromESPError(i));
            delay(1000);
        }
    }
    delay(1000);
    Serial.println("loop");

}

void TestApplication::handleException(std::runtime_error error)
{
    Serial.println("======= EXCEPTION ============");
    Serial.println(error.what());
    Serial.println("===================");
}

void TestApplication::onGotData(MeshManager::MessageData messageData)
{
    Serial.println("got data");
    Serial.println("MAC: " + MeshManager::macAddressToString(messageData.fromMacAddress));
    Serial.println("Len: " + String(messageData.dataLength));

    COPY_MESSAGE_TO_CUSTOM(MyMessage, messageData, msg);

    Serial.println("Msg prop name: " + String(msg.propertyName));
    Serial.println("Msg test: " + String(msg.test));
    Serial.println("Msg prop val: " + String(msg.propertyValue));
    Serial.println("Msg prop unit: " + String(msg.unitType));
}

void TestApplication::onDataSent(MeshManager::MessageReceipt messageReceipt)
{
    Serial.println("sent data");
    Serial.println("MAC: " + MeshManager::macAddressToString(messageReceipt.recipientMacAddress));
    Serial.println("Success: " + String(messageReceipt.success));
}
