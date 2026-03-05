//
// Created by Andrew Simmons on 2/25/26.
//

#ifndef FIRMWORK_TESTAPPLICATIONSETTINGS_H
#define FIRMWORK_TESTAPPLICATIONSETTINGS_H

#include <Settings.h>

class TestApplicationSettings : public Settings
{
public:

    SETTING(int, speed, 10, "Drive speed");
    SETTING(float, threshold, 1.25f, "Threshold");
    SETTING(bool, enabled, true, "Enabled");
    SETTING(String, deviceLabel, "truckESP32", "Device label");
};

#endif //FIRMWORK_TESTAPPLICATIONSETTINGS_H
