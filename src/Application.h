//
// Created by Andrew Simmons on 8/18/25.
//

#ifndef FIRMWORK_APPLICATION_H
#define FIRMWORK_APPLICATION_H


#include <stdexcept>


#define APP(AppType)                                          \
    Application* Application::application = new AppType();    \
    void setup() {                                            \
        try {                                                 \
            Application::application->setup();                \
        } catch (const std::runtime_error& e) {               \
            Application::application->handleException(e);     \
        }                                                     \
    }                                                         \
    void loop() {                                             \
        try {                                                 \
            Application::application->loop();                 \
        } catch (const std::runtime_error& e) {               \
            Application::application->handleException(e);     \
        }                                                     \
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
