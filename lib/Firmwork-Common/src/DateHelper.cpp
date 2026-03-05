#include <DateHelper.h>
#include <stdio.h>
#include <string.h>

DateHelper::TimeZoneConfig DateHelper::UTCConfig() {
    return {
        0,
        0,
        false,
        {3, 2, 0, 2},
        {11, 1, 0, 2},
        true,
        true,
        "UTC",
        "UTC"
    };
}

DateHelper::TimeZoneConfig DateHelper::USCentralConfig() {
    return {
        -360,
        -300,
        true,
        {3, 2, 0, 2},   // 2nd Sunday in March @ 02:00 local
        {11, 1, 0, 2},  // 1st Sunday in November @ 02:00 local
        true,
        true,
        "CST",
        "CDT"
    };
}

uint8_t DateHelper::nthWeekday(uint16_t year, uint8_t month, uint8_t nth, uint8_t weekday) {
    const DateTime firstOfMonth(year, month, 1, 0, 0, 0);
    const uint8_t firstDow = firstOfMonth.dayOfTheWeek();
    return static_cast<uint8_t>(1 + ((7 + weekday - firstDow) % 7) + ((nth - 1) * 7));
}

bool DateHelper::isDstFromUtc(const DateTime &utcTime, const TimeZoneConfig &timeZone) {
    if (!timeZone.useDst) {
        return false;
    }

    const uint16_t year = utcTime.year();
    const uint8_t startDay = nthWeekday(
        year,
        timeZone.dstStartLocal.month,
        timeZone.dstStartLocal.nth,
        timeZone.dstStartLocal.weekday);
    const uint8_t endDay = nthWeekday(
        year,
        timeZone.dstEndLocal.month,
        timeZone.dstEndLocal.nth,
        timeZone.dstEndLocal.weekday);

    const DateTime dstStartLocal(
        year,
        timeZone.dstStartLocal.month,
        startDay,
        timeZone.dstStartLocal.hour,
        0,
        0);
    const DateTime dstEndLocal(
        year,
        timeZone.dstEndLocal.month,
        endDay,
        timeZone.dstEndLocal.hour,
        0,
        0);

    const int16_t startOffsetMinutes =
        timeZone.dstStartUsesStandardOffset ? timeZone.standardOffsetMinutes : timeZone.daylightOffsetMinutes;
    const int16_t endOffsetMinutes =
        timeZone.dstEndUsesDaylightOffset ? timeZone.daylightOffsetMinutes : timeZone.standardOffsetMinutes;

    const DateTime dstStartUtc(dstStartLocal.unixtime() - (static_cast<int32_t>(startOffsetMinutes) * 60));
    const DateTime dstEndUtc(dstEndLocal.unixtime() - (static_cast<int32_t>(endOffsetMinutes) * 60));

    return inWindow(utcTime.unixtime(), dstStartUtc.unixtime(), dstEndUtc.unixtime());
}

bool DateHelper::isDstFromLocal(const DateTime &localTime, const TimeZoneConfig &timeZone) {
    if (!timeZone.useDst) {
        return false;
    }

    const uint16_t year = localTime.year();
    const uint8_t startDay = nthWeekday(
        year,
        timeZone.dstStartLocal.month,
        timeZone.dstStartLocal.nth,
        timeZone.dstStartLocal.weekday);
    const uint8_t endDay = nthWeekday(
        year,
        timeZone.dstEndLocal.month,
        timeZone.dstEndLocal.nth,
        timeZone.dstEndLocal.weekday);

    const DateTime dstStartLocal(
        year,
        timeZone.dstStartLocal.month,
        startDay,
        timeZone.dstStartLocal.hour,
        0,
        0);
    const DateTime dstEndLocal(
        year,
        timeZone.dstEndLocal.month,
        endDay,
        timeZone.dstEndLocal.hour,
        0,
        0);

    return inWindow(localTime.unixtime(), dstStartLocal.unixtime(), dstEndLocal.unixtime());
}

DateTime DateHelper::localFromUtc(const DateTime &utcTime, const TimeZoneConfig &timeZone) {
    const bool isDst = isDstFromUtc(utcTime, timeZone);
    const int16_t offsetMinutes =
        (timeZone.useDst && isDst) ? timeZone.daylightOffsetMinutes : timeZone.standardOffsetMinutes;
    return DateTime(utcTime.unixtime() + (static_cast<int32_t>(offsetMinutes) * 60));
}

DateTime DateHelper::utcFromLocal(const DateTime &localTime, const TimeZoneConfig &timeZone) {
    const bool isDst = isDstFromLocal(localTime, timeZone);
    const int16_t offsetMinutes =
        (timeZone.useDst && isDst) ? timeZone.daylightOffsetMinutes : timeZone.standardOffsetMinutes;
    return DateTime(localTime.unixtime() - (static_cast<int32_t>(offsetMinutes) * 60));
}

DateTime DateHelper::localFromBuildTime(const __FlashStringHelper *buildDate, const __FlashStringHelper *buildTime) {
    return DateTime(buildDate, buildTime);
}

DateTime DateHelper::utcFromBuildTime(
    const __FlashStringHelper *buildDate,
    const __FlashStringHelper *buildTime,
    const TimeZoneConfig &timeZone) {
    return utcFromLocal(localFromBuildTime(buildDate, buildTime), timeZone);
}

const char *DateHelper::alarmTypeLabel(AlarmRepeatType repeatType) {
    switch (repeatType) {
        case ALARM_ONE_TIME:
            return "ONE_TIME";
        case ALARM_DAILY:
            return "DAILY";
        case ALARM_WEEKLY:
            return "WEEKLY";
        case ALARM_WEEKDAYS_MASK:
            return "WEEKDAY_MASK";
        case ALARM_INTERVAL:
            return "INTERVAL";
        default:
            return "UNKNOWN";
    }
}

String DateHelper::getFormattedDateString(const DateTime &dateTime, const char *format, uint16_t milliseconds) {
    if (format == nullptr) {
        return String();
    }

    static const char *kWeekdayShort[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *kWeekdayFull[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    static const char *kMonthShort[12] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static const char *kMonthFull[12] = {"January",
                                         "February",
                                         "March",
                                         "April",
                                         "May",
                                         "June",
                                         "July",
                                         "August",
                                         "September",
                                         "October",
                                         "November",
                                         "December"};

    char year4[5];
    char year2[3];
    char month2[3];
    char day2[3];
    char hour2[3];
    char minute2[3];
    char second2[3];
    char millis3[4];

    const uint16_t clampedMillis = static_cast<uint16_t>(milliseconds % 1000U);
    const uint8_t weekday = static_cast<uint8_t>(dateTime.dayOfTheWeek() % 7U);
    const uint8_t monthRaw = dateTime.month();
    const uint8_t monthIndex =
        static_cast<uint8_t>((monthRaw >= 1U && monthRaw <= 12U) ? (monthRaw - 1U) : 0U);

    snprintf(year4, sizeof(year4), "%04u", dateTime.year());
    snprintf(year2, sizeof(year2), "%02u", static_cast<unsigned>(dateTime.year() % 100U));
    snprintf(month2, sizeof(month2), "%02u", dateTime.month());
    snprintf(day2, sizeof(day2), "%02u", dateTime.day());
    snprintf(hour2, sizeof(hour2), "%02u", dateTime.hour());
    snprintf(minute2, sizeof(minute2), "%02u", dateTime.minute());
    snprintf(second2, sizeof(second2), "%02u", dateTime.second());
    snprintf(millis3, sizeof(millis3), "%03u", clampedMillis);

    String output;
    output.reserve(strlen(format) + 20U);

    const size_t len = strlen(format);
    for (size_t i = 0; i < len;) {
        if ((i + 4U) <= len && strncmp(&format[i], "dddd", 4U) == 0) {
            output += kWeekdayFull[weekday];
            i += 4U;
            continue;
        }
        if ((i + 4U) <= len && strncmp(&format[i], "YYYY", 4U) == 0) {
            output += year4;
            i += 4U;
            continue;
        }
        if ((i + 4U) <= len && strncmp(&format[i], "MMMM", 4U) == 0) {
            output += kMonthFull[monthIndex];
            i += 4U;
            continue;
        }
        if ((i + 3U) <= len && strncmp(&format[i], "ddd", 3U) == 0) {
            output += kWeekdayShort[weekday];
            i += 3U;
            continue;
        }
        if ((i + 3U) <= len && strncmp(&format[i], "MMM", 3U) == 0) {
            output += kMonthShort[monthIndex];
            i += 3U;
            continue;
        }
        if ((i + 3U) <= len && strncmp(&format[i], "sss", 3U) == 0) {
            output += millis3;
            i += 3U;
            continue;
        }
        if ((i + 2U) <= len && strncmp(&format[i], "YY", 2U) == 0) {
            output += year2;
            i += 2U;
            continue;
        }
        if ((i + 2U) <= len && strncmp(&format[i], "DD", 2U) == 0) {
            output += day2;
            i += 2U;
            continue;
        }
        if ((i + 2U) <= len && strncmp(&format[i], "HH", 2U) == 0) {
            output += hour2;
            i += 2U;
            continue;
        }
        if ((i + 2U) <= len && strncmp(&format[i], "mm", 2U) == 0) {
            output += minute2;
            i += 2U;
            continue;
        }
        if ((i + 2U) <= len && strncmp(&format[i], "SS", 2U) == 0) {
            output += second2;
            i += 2U;
            continue;
        }
        if ((i + 2U) <= len && strncmp(&format[i], "MM", 2U) == 0) {
            const bool minuteLike = ((i > 0U) && (format[i - 1U] == ':')) || ((i + 2U) < len && (format[i + 2U] == ':'));
            output += minuteLike ? minute2 : month2;
            i += 2U;
            continue;
        }

        output += format[i];
        ++i;
    }

    return output;
}

bool DateHelper::isValidWeekdayIndex(uint8_t weekdayIndex) {
    return weekdayIndex <= 6U;
}

uint8_t DateHelper::weekdayBitFromIndex(uint8_t weekdayIndex) {
    if (!isValidWeekdayIndex(weekdayIndex)) {
        return 0U;
    }
    return static_cast<uint8_t>(1U << weekdayIndex);
}

bool DateHelper::isSingleWeekdayBit(uint8_t weekdayBit) {
    if ((weekdayBit & ALL_WEEKDAYS) == 0U) {
        return false;
    }

    if ((weekdayBit & ~ALL_WEEKDAYS) != 0U) {
        return false;
    }

    return (weekdayBit & static_cast<uint8_t>(weekdayBit - 1U)) == 0U;
}

bool DateHelper::weekdayIndexFromBit(uint8_t weekdayBit, uint8_t &weekdayIndexOut) {
    if (!isSingleWeekdayBit(weekdayBit)) {
        return false;
    }

    uint8_t index = 0U;
    uint8_t bit = weekdayBit;
    while ((bit & 0x01U) == 0U) {
        bit >>= 1;
        ++index;
    }

    weekdayIndexOut = index;
    return true;
}

uint32_t DateHelper::absDiffSeconds(uint32_t a, uint32_t b) {
    return (a > b) ? (a - b) : (b - a);
}

int32_t DateHelper::diffSeconds(const DateTime &a, const DateTime &b) {
    return static_cast<int32_t>(a.unixtime()) - static_cast<int32_t>(b.unixtime());
}

uint32_t DateHelper::minuteKey(const DateTime &dt) {
    return dt.unixtime() / 60U;
}

bool DateHelper::sameMinute(const DateTime &a, const DateTime &b) {
    return minuteKey(a) == minuteKey(b);
}

bool DateHelper::inWindow(uint32_t value, uint32_t start, uint32_t end) {
    if (start < end) {
        return value >= start && value < end;
    }

    // Handles windows that wrap around year boundary.
    return value >= start || value < end;
}
