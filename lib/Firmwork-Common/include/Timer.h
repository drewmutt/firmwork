//
// Created by Andrew Simmons on 8/3/24.
//

#ifndef FIRMWORK_TIMER_H
#define FIRMWORK_TIMER_H
#include <Arduino.h>
#include <Updateable.h>

/* Example:
 * void getThermoReading(Timer::TriggerData data) { ... }
 *
 * Timer *thermoTimer = new Timer(THERMO_DELAY_MSEC, getThermoReading);
 * ...
 * loop(){
 *  thermoTimer->update();
 */
class Timer:public Updateable
{
    public:

        struct TriggerData { unsigned long long count; Timer* timer; };

        Timer(unsigned long long delay, std::function<void(TriggerData)> cb, bool oneShot = false)
                : delayMSec(delay), triggerFunction(std::move(cb)), isOneShot(oneShot) {}

        void setTriggerFunction(std::function<void(TriggerData)> cb) { triggerFunction = std::move(cb); }

        unsigned long long int getLastTriggerMSec() const;
        void setLastTriggerMSec(unsigned long long int lastTriggerMSec);
        unsigned long long int getDelayMSec() const;
        void setDelayMSec(unsigned long long int pDelayMSec);
        void setTriggerFunction(void (*triggerFunction)(TriggerData));
        void update() override;
        unsigned long long int getTriggerCount() const;
//        void setTriggerCount(unsigned long long int triggerCount);
        boolean getEnabled() const;
        void setEnabled(boolean enabled);
        void setIsOneShot(bool aIsOneShot) { this->isOneShot = aIsOneShot; }

    private:
        unsigned long long delayMSec{};
        unsigned long long lastTriggerMSec{};
        unsigned long long triggerCount{};
        bool enabled{true};
        bool isOneShot{false};
        std::function<void(TriggerData)> triggerFunction;
};




#endif