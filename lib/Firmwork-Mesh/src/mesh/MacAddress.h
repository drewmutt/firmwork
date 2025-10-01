//
// Created by Andrew Simmons on 8/18/25.
//

#ifndef FIRMWORK_MACADDRESS_H
#define FIRMWORK_MACADDRESS_H
#include "Arduino.h"

typedef struct MacAddress
{
    uint8_t addressBytes[6];
} MacAddress;

#endif //FIRMWORK_MACADDRESS_H
