//
// Created by Andrew Simmons on 8/18/25.
//

#ifndef FIRMWORK_APPLICATION_H
#define FIRMWORK_APPLICATION_H


#include <stdexcept>
#include <vector>
#include "Updateable.h"
#include "Timer.h"


#define APP(AppType)                                          \
    Application* application = new AppType();    \
    void setup() {                                            \
        try {                                                 \
            application->setup();                \
        } catch (const std::runtime_error& e) {               \
            application->handleException(e);     \
        }                                                     \
    }                                                         \
    void loop() {                                             \
        try {                                                 \
            for(Updateable *updateable:application->updateables)           \
                updateable->update();                          \
            application->loop();                 \
        } catch (const std::runtime_error& e) {               \
            application->handleException(e);     \
        }                                                     \
    }

#define COPY_MESSAGE_TO_CUSTOM(CustomType, MessageDataVar, NewMessageVarName) \
CustomType NewMessageVarName{};                                                                               \
memcpy(&NewMessageVarName, MessageDataVar.message, MessageDataVar.dataLength)

class Application
{

    public:
        static Application *application;
        virtual void setup() = 0;
        virtual void loop() = 0;
        virtual void handleException(std::runtime_error error) = 0;
        std::vector<Updateable *> updateables;
        void addUpdateable(Updateable *updateable){
            updateables.push_back(updateable);
        }
        Timer *createAndScheduleTimer(unsigned long long int delay, void (*aTriggerFunction)(Timer::TriggerData))
        {
            Timer *timer = new Timer(delay, aTriggerFunction);
            this->addUpdateable(timer);
            return timer;
        }

    protected:

};


#endif //FIRMWORK_APPLICATION_H
