//
// Created by Andrew Simmons on 5/9/23.
//

#ifndef ROBOTOPO_MATHHELPER_H
#define ROBOTOPO_MATHHELPER_H


typedef struct ULLongRange { unsigned long long min, max; } ULongRange;
typedef struct FloatRange { float min, max; } FloatRange;
typedef struct LRange{ long min, max; } LRange;

class MathHelper
{
    public:
        static double map(double x, double in_min, double in_max, double out_min, double out_max);
        long LLongMap(long long int x, long long int in_min, long long int in_max, long long int out_min, long long int out_max);
        unsigned long ULongMap(unsigned long x, unsigned long in_min, unsigned long in_max, unsigned long out_min, unsigned long out_max);
        long LongMap(long x, long in_min, long in_max, long out_min, long out_max);
        long LRangeMap(long x, LRange inRange, LRange outRange);
        float FRangeMap(float x, FloatRange inRange, FloatRange outRange);
        float FloatMap(float x, float in_min, float in_max, float out_min, float out_max);
};


#endif //ROBOTOPO_MATHHELPER_H
