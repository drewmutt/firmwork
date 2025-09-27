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


// Method sig be like:
// void onTakeSonarReading(unsigned long long triggerCount, Timer *timer)
void Timer::setTriggerFunction(void (*pTriggerFunction)(TriggerData))
{
    Timer::triggerFunction = pTriggerFunction;
}

void Timer::update()
{
    if(!enabled)
        return;

    unsigned long now = millis();
    unsigned long long elapsed = now - this->lastTriggerMSec;
    if(elapsed >= delayMSec)
    {
        this->lastTriggerMSec = now;
        TriggerData data = {triggerCount++, this};
        if (triggerFunction)
            triggerFunction(data);
        if(isOneShot)
            enabled = false;
    }
}

unsigned long long int Timer::getTriggerCount() const
{
    return triggerCount;
}

boolean Timer::getEnabled() const
{
    return enabled;
}

void Timer::setEnabled(boolean pEnabled)
{
    Timer::enabled = pEnabled;
}
