#include <RtcAlarmScheduler.h>

#include <limits.h>

RtcAlarmScheduler::RtcAlarmScheduler(RtcManager &rtcManager) : rtc_(rtcManager) {
    clear();
}

void RtcAlarmScheduler::clear() {
    alarmCount_ = 0;
    for (size_t i = 0; i < kMaxAlarms; ++i) {
        alarms_[i] = {};
        alarms_[i].lastTriggerMinuteKey = UINT32_MAX;
    }
}

bool RtcAlarmScheduler::remove(uint32_t alarmId) {
    for (size_t i = 0; i < alarmCount_; ++i) {
        if (alarms_[i].id != alarmId) {
            continue;
        }

        for (size_t j = i + 1; j < alarmCount_; ++j) {
            alarms_[j - 1] = alarms_[j];
        }
        --alarmCount_;
        return true;
    }

    return false;
}

bool RtcAlarmScheduler::setEnabled(uint32_t alarmId, bool enabled) {
    AlarmDefinition *alarm = findAlarm(alarmId);
    if (alarm == nullptr) {
        return false;
    }

    alarm->enabled = enabled;
    if (!enabled) {
        alarm->lastTriggerMinuteKey = UINT32_MAX;
    }

    return true;
}

void RtcAlarmScheduler::setDefaultCallback(std::function<void(AlarmEventData)> callback) {
    defaultCallback_ = std::move(callback);
}

uint32_t RtcAlarmScheduler::addOneTimeLocal(
    const DateTime &localTime,
    std::function<void(AlarmEventData)> onTrigger) {
    const uint32_t id = allocateAlarm(ALARM_ONE_TIME, std::move(onTrigger));
    AlarmDefinition *alarm = findAlarm(id);
    if (alarm == nullptr) {
        return 0;
    }

    alarm->oneTimeLocal = localTime;
    return id;
}

uint32_t RtcAlarmScheduler::addDaily(
    uint8_t hour24,
    uint8_t minute,
    std::function<void(AlarmEventData)> onTrigger) {
    const uint32_t id = allocateAlarm(ALARM_DAILY, std::move(onTrigger));
    AlarmDefinition *alarm = findAlarm(id);
    if (alarm == nullptr) {
        return 0;
    }

    alarm->hour24 = hour24;
    alarm->minute = minute;
    return id;
}

uint32_t RtcAlarmScheduler::addWeekly(
    uint8_t weekday,
    uint8_t hour24,
    uint8_t minute,
    std::function<void(AlarmEventData)> onTrigger) {
    uint8_t normalizedWeekday = 0U;
    if (!normalizeWeekday(weekday, normalizedWeekday)) {
        return 0;
    }

    const uint32_t id = allocateAlarm(ALARM_WEEKLY, std::move(onTrigger));
    AlarmDefinition *alarm = findAlarm(id);
    if (alarm == nullptr) {
        return 0;
    }

    alarm->weekday = normalizedWeekday;
    alarm->hour24 = hour24;
    alarm->minute = minute;
    return id;
}

uint32_t RtcAlarmScheduler::addWeekdayMask(
    uint8_t weekdayMask,
    uint8_t hour24,
    uint8_t minute,
    std::function<void(AlarmEventData)> onTrigger) {
    const uint8_t normalizedMask = static_cast<uint8_t>(weekdayMask & DateHelper::ALL_WEEKDAYS);
    if (normalizedMask == 0U) {
        return 0;
    }

    const uint32_t id = allocateAlarm(ALARM_WEEKDAYS_MASK, std::move(onTrigger));
    AlarmDefinition *alarm = findAlarm(id);
    if (alarm == nullptr) {
        return 0;
    }

    alarm->weekdayMask = normalizedMask;
    alarm->hour24 = hour24;
    alarm->minute = minute;
    return id;
}

uint32_t RtcAlarmScheduler::addInterval(
    uint32_t intervalSeconds,
    bool fireImmediately,
    std::function<void(AlarmEventData)> onTrigger) {
    if (intervalSeconds == 0U) {
        return 0;
    }

    const uint32_t id = allocateAlarm(ALARM_INTERVAL, std::move(onTrigger));
    AlarmDefinition *alarm = findAlarm(id);
    if (alarm == nullptr) {
        return 0;
    }

    alarm->intervalSeconds = intervalSeconds;

    DateTime utcNow;
    if (!rtc_.nowUtc(utcNow)) {
        alarm->nextTriggerUtc = 0;
    } else {
        alarm->nextTriggerUtc = fireImmediately ? utcNow.unixtime() : (utcNow.unixtime() + intervalSeconds);
    }

    return id;
}

void RtcAlarmScheduler::update() {
    DateTime localNow;
    DateTime utcNow;
    if (!rtc_.nowLocal(localNow) || !rtc_.nowUtc(utcNow)) {
        return;
    }

    const uint32_t nowMinuteKey = DateHelper::minuteKey(localNow);
    for (size_t i = 0; i < alarmCount_; ++i) {
        AlarmDefinition &alarm = alarms_[i];
        if (!alarm.enabled) {
            continue;
        }

        if (shouldTrigger(alarm, localNow, utcNow, nowMinuteKey)) {
            triggerAlarm(alarm, localNow, utcNow, nowMinuteKey);
        }
    }
}

uint32_t RtcAlarmScheduler::secondsUntilNextAlarm() {
    DateTime localNow;
    DateTime utcNow;
    if (!rtc_.nowLocal(localNow) || !rtc_.nowUtc(utcNow)) {
        return UINT32_MAX;
    }

    uint32_t best = UINT32_MAX;
    for (size_t i = 0; i < alarmCount_; ++i) {
        const AlarmDefinition &alarm = alarms_[i];
        if (!alarm.enabled) {
            continue;
        }

        const uint32_t diff = secondsUntilAlarm(alarm, localNow, utcNow);

        if (diff < best) {
            best = diff;
        }
    }

    return best;
}

size_t RtcAlarmScheduler::getAlarmDefinitionsSortedByNextTrigger(AlarmDefinitionWithSchedule *outAlarms, size_t maxOut) {
    if (outAlarms == nullptr || maxOut == 0U) {
        return 0;
    }

    DateTime localNow;
    DateTime utcNow;
    const bool hasNow = rtc_.nowLocal(localNow) && rtc_.nowUtc(utcNow);

    AlarmDefinitionWithSchedule sorted[kMaxAlarms];
    for (size_t i = 0; i < alarmCount_; ++i) {
        sorted[i] = {};
        sorted[i].definition = alarms_[i];
        sorted[i].secondsUntilTrigger = hasNow ? secondsUntilAlarm(alarms_[i], localNow, utcNow) : UINT32_MAX;
    }

    // Selection sort by nearest trigger first; ties resolved by alarm id.
    for (size_t i = 0; i < alarmCount_; ++i) {
        size_t best = i;
        for (size_t j = i + 1; j < alarmCount_; ++j) {
            if (sorted[j].secondsUntilTrigger < sorted[best].secondsUntilTrigger) {
                best = j;
            } else if (
                sorted[j].secondsUntilTrigger == sorted[best].secondsUntilTrigger &&
                sorted[j].definition.id < sorted[best].definition.id) {
                best = j;
            }
        }

        if (best != i) {
            const AlarmDefinitionWithSchedule temp = sorted[i];
            sorted[i] = sorted[best];
            sorted[best] = temp;
        }
    }

    const size_t outCount = (alarmCount_ < maxOut) ? alarmCount_ : maxOut;
    for (size_t i = 0; i < outCount; ++i) {
        outAlarms[i] = sorted[i];
    }

    return outCount;
}

RtcAlarmScheduler::AlarmDefinition *RtcAlarmScheduler::findAlarm(uint32_t alarmId) {
    if (alarmId == 0U) {
        return nullptr;
    }

    for (size_t i = 0; i < alarmCount_; ++i) {
        if (alarms_[i].id == alarmId) {
            return &alarms_[i];
        }
    }

    return nullptr;
}

uint32_t RtcAlarmScheduler::allocateAlarm(
    RepeatType repeatType,
    std::function<void(AlarmEventData)> onTrigger) {
    if (alarmCount_ >= kMaxAlarms) {
        return 0;
    }

    AlarmDefinition &alarm = alarms_[alarmCount_++];
    alarm = {};
    alarm.id = nextAlarmId_++;
    alarm.repeatType = repeatType;
    alarm.enabled = true;
    alarm.lastTriggerMinuteKey = UINT32_MAX;
    alarm.onTrigger = std::move(onTrigger);
    return alarm.id;
}

bool RtcAlarmScheduler::normalizeWeekday(uint8_t weekdayInput, uint8_t &weekdayIndexOut) {
    if (DateHelper::isValidWeekdayIndex(weekdayInput)) {
        weekdayIndexOut = weekdayInput;
        return true;
    }

    return DateHelper::weekdayIndexFromBit(weekdayInput, weekdayIndexOut);
}

uint32_t RtcAlarmScheduler::secondsUntilAlarm(
    const AlarmDefinition &alarm,
    const DateTime &localNow,
    const DateTime &utcNow) {
    if (!alarm.enabled) {
        return UINT32_MAX;
    }

    switch (alarm.repeatType) {
        case ALARM_ONE_TIME: {
            const DateTime oneTimeUtc = DateHelper::utcFromLocal(alarm.oneTimeLocal, rtc_.getTimeZone());
            if (oneTimeUtc.unixtime() <= utcNow.unixtime()) {
                return 0;
            }
            return oneTimeUtc.unixtime() - utcNow.unixtime();
        }
        case ALARM_DAILY:
        case ALARM_WEEKLY:
        case ALARM_WEEKDAYS_MASK: {
            const DateTime todayAtTime(
                localNow.year(),
                localNow.month(),
                localNow.day(),
                alarm.hour24,
                alarm.minute,
                0);

            for (uint8_t d = 0; d <= 7; ++d) {
                const DateTime candidate(todayAtTime.unixtime() + (static_cast<uint32_t>(d) * 86400U));
                if (candidate.unixtime() < localNow.unixtime()) {
                    continue;
                }

                bool dayMatch = false;
                if (alarm.repeatType == ALARM_DAILY) {
                    dayMatch = true;
                } else if (alarm.repeatType == ALARM_WEEKLY) {
                    dayMatch = (candidate.dayOfTheWeek() == alarm.weekday);
                } else {
                    const uint8_t dayBit = static_cast<uint8_t>(1U << candidate.dayOfTheWeek());
                    dayMatch = (alarm.weekdayMask & dayBit) != 0;
                }

                if (dayMatch) {
                    return candidate.unixtime() - localNow.unixtime();
                }
            }
            return UINT32_MAX;
        }
        case ALARM_INTERVAL:
            if (alarm.intervalSeconds == 0U) {
                return UINT32_MAX;
            }
            if (alarm.nextTriggerUtc <= utcNow.unixtime()) {
                return 0;
            }
            return alarm.nextTriggerUtc - utcNow.unixtime();
    }

    return UINT32_MAX;
}

bool RtcAlarmScheduler::shouldTrigger(
    const AlarmDefinition &alarm,
    const DateTime &localNow,
    const DateTime &utcNow,
    uint32_t minuteKey) {
    if (alarm.repeatType != ALARM_INTERVAL && alarm.lastTriggerMinuteKey == minuteKey) {
        return false;
    }

    switch (alarm.repeatType) {
        case ALARM_ONE_TIME: {
            const DateTime oneTimeUtc = DateHelper::utcFromLocal(alarm.oneTimeLocal, rtc_.getTimeZone());
            return utcNow.unixtime() >= oneTimeUtc.unixtime();
        }
        case ALARM_DAILY:
            return (localNow.hour() == alarm.hour24) && (localNow.minute() == alarm.minute);
        case ALARM_WEEKLY:
            return (localNow.dayOfTheWeek() == alarm.weekday) && (localNow.hour() == alarm.hour24) &&
                   (localNow.minute() == alarm.minute);
        case ALARM_WEEKDAYS_MASK: {
            const uint8_t dayBit = static_cast<uint8_t>(1U << localNow.dayOfTheWeek());
            return ((alarm.weekdayMask & dayBit) != 0) && (localNow.hour() == alarm.hour24) &&
                   (localNow.minute() == alarm.minute);
        }
        case ALARM_INTERVAL:
            if (alarm.intervalSeconds == 0U) {
                return false;
            }
            return utcNow.unixtime() >= alarm.nextTriggerUtc;
    }

    return false;
}

void RtcAlarmScheduler::triggerAlarm(
    AlarmDefinition &alarm,
    const DateTime &localNow,
    const DateTime &utcNow,
    uint32_t minuteKey) {
    alarm.lastTriggerMinuteKey = minuteKey;

    AlarmEventData eventData = {
        alarm.id,
        alarm.repeatType,
        localNow,
        utcNow,
    };

    if (alarm.onTrigger) {
        alarm.onTrigger(eventData);
    } else if (defaultCallback_) {
        defaultCallback_(eventData);
    }

    if (alarm.repeatType == ALARM_ONE_TIME) {
        alarm.enabled = false;
    }

    if (alarm.repeatType == ALARM_INTERVAL && alarm.intervalSeconds > 0U) {
        uint32_t nextUtc = alarm.nextTriggerUtc;
        if (nextUtc == 0U) {
            nextUtc = utcNow.unixtime();
        }

        do {
            nextUtc += alarm.intervalSeconds;
        } while (nextUtc <= utcNow.unixtime());

        alarm.nextTriggerUtc = nextUtc;
    }
}
