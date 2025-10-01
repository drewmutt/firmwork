//
// Created by Andrew Simmons on 5/9/23.
//

#ifndef ROBOTOPO_STEPPERMANAGER_H
#define ROBOTOPO_STEPPERMANAGER_H
#include <AccelStepper.h>

typedef enum StepperMode
{
    STEPPER_NONE = 'n',
    STEPPER_MOVE_TO = 't',
    STEPPER_MOVE_SPEED = 's',
} StepperMode;

typedef enum LimitMode
{
    LIMIT_NONE,
    LIMIT_HIGH,
    LIMIT_LOW,
} LimitMode;

class StepperManager
{

        AccelStepper *stepper;
    StepperMode mode = STEPPER_NONE;
    public:
        explicit StepperManager(AccelStepper *stepper);
        long currentPosition();
//        void setSpeed(float speed);
        void setMaxSpeed(float speed);
        void setAcceleration(float acc);
        void moveToAbsolute(long pos);
        void setCurrentPosition(long pos);
        float speed();
        bool run(bool overrideLimits = false);
        long targetPosition();
        long distanceToGo();
        void moveRelative(long pos);
        void moveAtSpeed(float speed);
        void stop();
        LimitMode limitMode = LIMIT_NONE;
        boolean (*limitFunction)(void) = nullptr;
        StepperManager(AccelStepper *pStepper, boolean (*pLimitFunction)(void), LimitMode limitMode);
        void moveRelative(long pos, float speed);
        void moveToAbsolute(long pos, float speed);
        void softStop();
};


#endif //ROBOTOPO_STEPPERMANAGER_H
