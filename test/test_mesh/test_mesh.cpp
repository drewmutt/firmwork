//
// Created by Andrew Simmons on 8/17/25.
//
#include <Arduino.h>
#include <unity.h>
#include <mesh/MeshManager.h>

MeshManager *meshManager;
void setUp(void)
{
     meshManager = new MeshManager();
}

void tearDown(void)
{
    // clean stuff up here
}


void test_mesh(void)
{
    MeshManager::MacAddress macAddress = MeshManager::getSelfMacAddress();
    TEST_ASSERT_NOT_EQUAL(macAddress.addressBytes[0], 0);
}

void setup()
{
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);
    UNITY_BEGIN(); // IMPORTANT LINE!
}


void loop()
{
    RUN_TEST(test_mesh);
    UNITY_END(); // stop unit testing
}