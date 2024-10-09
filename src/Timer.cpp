//
// Created by Andrew Simmons on 8/3/24.
//

#include "Timer.h"


unsigned long long int Timer::getLastTriggerMSec() const
{
    return lastTriggerMSec;
}

void Timer::setLastTriggerMSec(unsigned long long int pLastTriggerMSec)
{
    Timer::lastTriggerMSec = pLastTriggerMSec;
}

unsigned long long int Timer::getDelayMSec() const
{
    return delayMSec;
}

void Timer::setDelayMSec(unsigned long long int pDelayMSec)
{
    Timer::delayMSec = pDelayMSec;
}


void Timer::setTriggerFunction(void (*pTriggerFunction)(unsigned long long, Timer *))
{
    Timer::triggerFunction = pTriggerFunction;
}

bool Timer::update()
{
    unsigned long now = millis();
    unsigned long long elapsed = now - this->lastTriggerMSec;
    if(elapsed > delayMSec)
    {
        this->lastTriggerMSec = now;
        if(enabled)
        {
            triggerFunction(triggerCount++, this);
        }
        return true;
    }
    return false;
}

unsigned long long int Timer::getTriggerCount() const
{
    return triggerCount;
}

void Timer::setTriggerCount(unsigned long long int pTriggerCount)
{
    Timer::triggerCount = pTriggerCount;
}

boolean Timer::getEnabled() const
{
    return enabled;
}

void Timer::setEnabled(boolean pEnabled)
{
    Timer::enabled = pEnabled;
}
