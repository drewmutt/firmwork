//
// Date/time helper utilities for RTC services.
//

#ifndef FIRMWORK_DATEHELPER_H
#define FIRMWORK_DATEHELPER_H

#include <Arduino.h>
#include <RTClib.h>
#include <stdint.h>

class DateHelper {
public:
    TimeSpan static minutesTimeSpan(int8_t minutes) { return TimeSpan(0, 0, minutes, 0); }
    TimeSpan static secondsTimeSpan(int8_t seconds) { return TimeSpan(0, 0, 0, seconds); }
    TimeSpan static hoursTimeSpan(int8_t hours) { return TimeSpan(0, hours, 0, 0); }
    TimeSpan static daysTimeSpan(int8_t days) { return TimeSpan(days, 0, 0, 0); }
    // Weekday bit flags. Can be OR'd for masks.
    enum Weekday : uint8_t {
        SUNDAY = (1U << 0),
        MONDAY = (1U << 1),
        TUESDAY = (1U << 2),
        WEDNESDAY = (1U << 3),
        THURSDAY = (1U << 4),
        FRIDAY = (1U << 5),
        SATURDAY = (1U << 6),
        ALL_WEEKDAYS = static_cast<uint8_t>(
            SUNDAY | MONDAY | TUESDAY | WEDNESDAY | THURSDAY | FRIDAY | SATURDAY)
    };

    enum AlarmRepeatType : uint8_t {
        ALARM_ONE_TIME = 0,
        ALARM_DAILY = 1,
        ALARM_WEEKLY = 2,
        ALARM_WEEKDAYS_MASK = 3,
        ALARM_INTERVAL = 4,
    };

    struct DstRule {
        uint8_t month;
        uint8_t nth;
        uint8_t weekday;  // 0=Sunday..6=Saturday
        uint8_t hour;
    };

    struct TimeZoneConfig {
        int16_t standardOffsetMinutes;  // e.g. -360 for UTC-6
        int16_t daylightOffsetMinutes;  // e.g. -300 for UTC-5
        bool useDst;
        DstRule dstStartLocal;
        DstRule dstEndLocal;
        bool dstStartUsesStandardOffset;
        bool dstEndUsesDaylightOffset;
        const char *standardName;
        const char *daylightName;
    };



    static TimeZoneConfig UTCConfig();
    static TimeZoneConfig USCentralConfig();

    static uint8_t nthWeekday(uint16_t year, uint8_t month, uint8_t nth, uint8_t weekday);
    static bool isDstFromUtc(const DateTime &utcTime, const TimeZoneConfig &timeZone);
    static bool isDstFromLocal(const DateTime &localTime, const TimeZoneConfig &timeZone);
    static DateTime localFromUtc(const DateTime &utcTime, const TimeZoneConfig &timeZone);
    static DateTime utcFromLocal(const DateTime &localTime, const TimeZoneConfig &timeZone);

    static DateTime localFromBuildTime(const __FlashStringHelper *buildDate, const __FlashStringHelper *buildTime);
    static DateTime utcFromBuildTime(
        const __FlashStringHelper *buildDate,
        const __FlashStringHelper *buildTime,
        const TimeZoneConfig &timeZone);
    static const char *alarmTypeLabel(AlarmRepeatType repeatType);
    static String getFormattedDateString(const DateTime &dateTime, const char *format, uint16_t milliseconds = 0);
    static DateTime addSeconds(const DateTime &dateTime, int32_t seconds);
    static DateTime addMinutes(const DateTime &dateTime, int32_t minutes);
    static DateTime addHours(const DateTime &dateTime, int32_t hours);
    static DateTime addDays(const DateTime &dateTime, int32_t days);
    static DateTime subtractSeconds(const DateTime &dateTime, uint32_t seconds);
    static DateTime subtractMinutes(const DateTime &dateTime, uint32_t minutes);
    static DateTime subtractHours(const DateTime &dateTime, uint32_t hours);
    static DateTime subtractDays(const DateTime &dateTime, uint32_t days);

    static bool isValidWeekdayIndex(uint8_t weekdayIndex);
    static uint8_t weekdayBitFromIndex(uint8_t weekdayIndex);
    static bool isSingleWeekdayBit(uint8_t weekdayBit);
    static bool weekdayIndexFromBit(uint8_t weekdayBit, uint8_t &weekdayIndexOut);

    static uint32_t absDiffSeconds(uint32_t a, uint32_t b);
    static int32_t diffSeconds(const DateTime &a, const DateTime &b);
    static uint32_t minuteKey(const DateTime &dt);
    static bool sameMinute(const DateTime &a, const DateTime &b);

private:
    static DateTime shiftSecondsClamped(const DateTime &dateTime, int64_t deltaSeconds);
    static bool inWindow(uint32_t value, uint32_t start, uint32_t end);
};

#endif  // FIRMWORK_DATEHELPER_H
