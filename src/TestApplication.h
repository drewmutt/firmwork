//
// Created by Andrew Simmons on 8/18/25.
//

#ifndef FIRMWORK_TESTAPPLICATION_H
#define FIRMWORK_TESTAPPLICATION_H
#include "Application.h"
#include "mesh/MacAddress.h"
#include "mesh/MeshManager.h"
#include <Firmwork/M5Graphics.h>
#include <Firmwork/GraphicsHelper.h>

class TestApplication: public Application
{
        void setup() override;
        void loop() override;
        void handleException(std::runtime_error error) override;
        MeshManager *meshManager = new MeshManager();

    public:
        static void onGotData(MeshManager::MessageData messageData);
        static void onDataSent(MeshManager::MessageReceipt messageReceipt);
};


#endif //FIRMWORK_TESTAPPLICATION_H
