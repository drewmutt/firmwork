//
// Created by Andrew Simmons on 8/3/24.
//

#ifndef FIRMWORK_TIMER_H
#define FIRMWORK_TIMER_H
#include <Arduino.h>
/* Example:
 * void getThermoReading(unsigned long long int, Timer *timer) { ... }
 *
 * Timer *thermoTimer = new Timer(THERMO_DELAY_MSEC, getThermoReading);
 * ...
 * loop(){
 *  thermoTimer->update();
 */
class Timer
{
    public:
        explicit Timer(unsigned long long int delay)
        {
            this->delayMSec = delay;
        }

        explicit Timer(unsigned long long int delay, void (*aTriggerFunction)(unsigned long long int, Timer *))
        {
            this->triggerFunction = aTriggerFunction;
            this->delayMSec = delay;
        }

        unsigned long long int getLastTriggerMSec() const;
        void setLastTriggerMSec(unsigned long long int lastTriggerMSec);
        unsigned long long int getDelayMSec() const;
        void setDelayMSec(unsigned long long int pDelayMSec);
        void setTriggerFunction(void (*triggerFunction)(unsigned long long, Timer *));
        void (*getTriggerFunction())(unsigned long long, Timer*) {return triggerFunction;}
        bool update();
        unsigned long long int getTriggerCount() const;
        void setTriggerCount(unsigned long long int triggerCount);
        boolean getEnabled() const;
        void setEnabled(boolean enabled);
    private:
        unsigned long long lastTriggerMSec = 0;
        unsigned long long delayMSec = 0;
        void (*triggerFunction)(unsigned long long, Timer*);
        unsigned long long triggerCount = 0;
        boolean enabled = true;
};


#endif //ICEMAKERHACK_TIMER_H