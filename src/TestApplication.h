//
// Created by Andrew Simmons on 8/18/25.
//

#ifndef FIRMWORK_TESTAPPLICATION_H
#define FIRMWORK_TESTAPPLICATION_H
#include "mesh/MacAddress.h"
#include <Firmwork/Graphics.h>
#include "mesh/MeshManager.h"
#include <Firmwork/M5Graphics.h>
#include <Firmwork/Colors.h>
#include "mesh/MacAddress.h"
#include "mesh/MeshManager.h"
#include <Firmwork/M5Graphics.h>
#include <Firmwork/ui/UIElement.h>
// #include <Firmwork/Bounds.h>
#include <Firmwork/GrayU8G2Graphics.h>
#include <../lib/Firmwork-Graphics-Core/include/Firmwork/ui/UIElement.h>
#include <Firmwork/ui/MenuUIElement.h>
#include <Firmwork/ui/connectors/RotaryEncoderToSelectableConnector.h>
#include <Firmwork/ui/Selectable.h>
#include <Firmwork/ui/MenuItemUIElement.h>
#include <Firmwork/motion/StepperManager.h>
#include <Firmwork/ui/TextUIElement.h>
#include "../lib/Firmwork-Common/include/Application.h"
#include "Log.h"

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
