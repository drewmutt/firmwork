//
// Created by Andrew Simmons on 8/18/25.
//

#ifndef FIRMWORK_APPLICATION_H
#define FIRMWORK_APPLICATION_H


#include <stdexcept>


// Define once (e.g., in a reusable header)
#define APP(AppType)                                          \
    Application *application = new AppType();                            \
    void setup() {                                                           \
        try {                                                                \
            application->setup();                               \
        } catch (const std::runtime_error& e) {                              \
            application->handleException(e);                    \
        }                                                                    \
    }                                                                        \
    void loop() {                                                            \
        try {                                                                \
            application->loop();                                \
        } catch (const std::runtime_error& e) {                              \
            application->handleException(e);                    \
        }                                                                    \
    }


class Application
{

    public:
        static Application *application;
        virtual void setup() = 0;
        virtual void loop() = 0;
        virtual void handleException(std::runtime_error error) = 0;
};


#endif //FIRMWORK_APPLICATION_H
