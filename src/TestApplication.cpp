//
// Created by Andrew Simmons on 8/18/25.
//

#include <HardwareSerial.h>
#include "TestApplication.h"
#include "mesh/MeshNode.h"


#define MASTER true

MeshNode MasterMeshNode({0x78, 0x21, 0x84, 0x89, 0x60, 0x74});

struct MyMessage:public Message
{
    int temp;
};

void TestApplication::setup()
{
    Serial.begin(9600);
    delay(2000);
    Serial.println("Role: " + String(MASTER==true?"MASTER":"SLAVE"));;

    meshManager->startESPNow();

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
    Serial.println("hey");
    delay(5000);

    if(!MASTER)
    {
        MyMessage msg{};
        msg.temp = 91;
        msg.test = 43;
        meshManager->sendMessage(&MasterMeshNode, msg);
        Serial.println("sent message");
    }
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

    MyMessage msg{};
    memcpy(&msg, &messageData.message, (int)sizeof(MyMessage));

    Serial.println("Msg temp: " + String(msg.temp));
    Serial.println("Msg test: " + String(msg.test));
}

void TestApplication::onDataSent(MeshManager::MessageReceipt messageReceipt)
{
    Serial.println("sent data");
    Serial.println("MAC: " + MeshManager::macAddressToString(messageReceipt.recipientMacAddress));
    Serial.println("Success: " + String(messageReceipt.success));
}
