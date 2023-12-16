//
// Created by Andrew Simmons on 5/9/23.
//

#include "StepperManager.h"

StepperManager::StepperManager(AccelStepper *stepper) : stepper(stepper)
{

}

//void StepperManager::setSpeed(float speed)
//{
//    stepper->setSpeed(speed);
//}

void StepperManager::setMaxSpeed(float speed)
{
    stepper->setMaxSpeed(speed);
}

void StepperManager::setAcceleration(float acc)
{
    stepper->setAcceleration(acc);
}

void StepperManager::moveToAbsolute(long pos, float speed)
{
    mode = STEPPER_MOVE_TO;
    stepper->moveTo(pos);
    stepper->setMaxSpeed(speed);
    stepper->setSpeed(speed);
}

void StepperManager::moveToAbsolute(long pos)
{
    mode = STEPPER_MOVE_TO;
    stepper->moveTo(pos);
}

void StepperManager::moveRelative(long pos)
{
    mode = STEPPER_MOVE_TO;
    stepper->move(pos);
}

void StepperManager::moveRelative(long pos, float speed)
{
    mode = STEPPER_MOVE_TO;
    stepper->move(pos);
    stepper->setSpeed(speed);
}

void StepperManager::moveAtSpeed(float speed)
{
    stepper->setSpeed(speed);
    mode = STEPPER_MOVE_SPEED;
}

void StepperManager::stop()
{
    stepper->stop();
    stepper->setSpeed(0);
    mode = STEPPER_NONE;
}

void StepperManager::softStop()
{
    stepper->stop();
//    stepper->setSpeed(0);
//    mode = STEPPER_NONE;
}

void StepperManager::setCurrentPosition(long pos)
{
    stepper->setCurrentPosition(pos);
}

float StepperManager::speed()
{
    return stepper->speed();
}

bool StepperManager::run(bool overrideLimits)
{
    if(limitFunction != nullptr && !overrideLimits)
    {
        if(limitFunction())
        {
            // Limit hit
            if(limitMode == LIMIT_LOW)
            {
                if((mode == STEPPER_MOVE_TO &&  (targetPosition() < currentPosition())) ||
                   (mode == STEPPER_MOVE_SPEED &&  speed() < 0)) {
                    stop(); // hmm
                    return false;
                }
            }
            else if(limitMode == LIMIT_HIGH)
            {
                if((mode == STEPPER_MOVE_TO &&  (targetPosition() > currentPosition())) ||
                   (mode == STEPPER_MOVE_SPEED &&  speed() > 0)) {
                    stop(); // hmm
                    return false;
                }
            }
        }
    }

    if(mode == STEPPER_MOVE_TO)
        stepper->run();
    else if(mode == STEPPER_MOVE_SPEED)
        stepper->runSpeed();

    return true;
}

long StepperManager::targetPosition()
{
    return stepper->targetPosition();
}

long StepperManager::distanceToGo()
{
    return stepper->distanceToGo();
}


long StepperManager::currentPosition()
{
    return stepper->currentPosition();
}

StepperManager::StepperManager(AccelStepper *pStepper, boolean (*pLimitFunction)(void), LimitMode pLimitMode)
{
    limitFunction = pLimitFunction;
    stepper = pStepper;
    limitMode = pLimitMode;
}


