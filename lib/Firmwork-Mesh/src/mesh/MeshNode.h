//
// Created by Andrew Simmons on 8/17/25.
//

#ifndef FIRMWORK_MESHNODE_H
#define FIRMWORK_MESHNODE_H


#include "MeshManager.h"
#include "MacAddress.h"

class MeshNode
{
    public:
        explicit MeshNode(const MacAddress &macAddress)
        {
            this->macAddress = macAddress;
        }
        const MacAddress &getMacAddress() const { return macAddress;}

    private:
        MacAddress macAddress;
};


#endif //FIRMWORK_MESHNODE_H
