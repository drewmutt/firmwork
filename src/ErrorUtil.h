//
// Created by Andrew Simmons on 2021-03-04.
//

#ifndef ARDUINO_CLION_MINIMAL_ERRORUTIL_H
#define ARDUINO_CLION_MINIMAL_ERRORUTIL_H


#include <Arduino.h>
#include <esp_err.h>

class ErrorUtil
{
public:
	static String getDescriptionFromESPError(esp_err_t error);
    static String getDescriptionFromWLStatus(wl_status_t error);
};



#endif //ARDUINO_CLION_MINIMAL_ERRORUTIL_H
