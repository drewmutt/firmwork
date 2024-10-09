//
// Created by Andrew Simmons on 5/9/23.
//

#include <Arduino.h>
#include "MathHelper.h"

double MathHelper::map(double x, double in_min, double in_max, double out_min, double out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long MathHelper::LLongMap(long long x, long long in_min, long long in_max, long long out_min, long long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

unsigned long MathHelper::ULongMap(unsigned  long x, unsigned  long in_min, unsigned long in_max, unsigned long out_min, unsigned long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long MathHelper::LongMap(long x,long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long MathHelper::LRangeMap(long x, LRange inRange, LRange outRange) {
    return LongMap(x, inRange.min, inRange.max, outRange.min, outRange.max);
}

float MathHelper::FloatMap(float x,float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float MathHelper::FRangeMap(float x, FloatRange inRange, FloatRange outRange) {
    return FloatMap(x, inRange.min, inRange.max, outRange.min, outRange.max);
}

