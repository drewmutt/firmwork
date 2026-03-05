//
// Alarm scheduler built on top of RtcManager.
//

#ifndef FIRMWORK_RTCALARMSCHEDULER_H
#define FIRMWORK_RTCALARMSCHEDULER_H

#include <DateHelper.h>
#include <RtcManager.h>
#include <functional>
#include <stddef.h>
#include <stdint.h>

class RtcAlarmScheduler {
public:
    using RepeatType = DateHelper::AlarmRepeatType;
    static constexpr RepeatType ALARM_ONE_TIME = DateHelper::ALARM_ONE_TIME;
    static constexpr RepeatType ALARM_DAILY = DateHelper::ALARM_DAILY;
    static constexpr RepeatType ALARM_WEEKLY = DateHelper::ALARM_WEEKLY;
    static constexpr RepeatType ALARM_WEEKDAYS_MASK = DateHelper::ALARM_WEEKDAYS_MASK;
    static constexpr RepeatType ALARM_INTERVAL = DateHelper::ALARM_INTERVAL;

    struct AlarmEventData {
        uint32_t alarmId;
        RepeatType repeatType;
        DateTime localTime;
        DateTime utcTime;
    };

    struct AlarmDefinition {
        uint32_t id;
        RepeatType repeatType;
        bool enabled;

        DateTime oneTimeLocal;
        uint8_t hour24;
        uint8_t minute;
        uint8_t weekday;
        uint8_t weekdayMask;  // bit0=Sunday ... bit6=Saturday

        uint32_t intervalSeconds;
        uint32_t nextTriggerUtc;
        uint32_t lastTriggerMinuteKey;

        std::function<void(AlarmEventData)> onTrigger;
    };

    struct AlarmDefinitionWithSchedule {
        AlarmDefinition definition;
        uint32_t secondsUntilTrigger;  // UINT32_MAX means unknown/no upcoming trigger
    };

    explicit RtcAlarmScheduler(RtcManager &rtcManager);

    void clear();
    bool remove(uint32_t alarmId);
    bool setEnabled(uint32_t alarmId, bool enabled);
    void setDefaultCallback(std::function<void(AlarmEventData)> callback);

    uint32_t addOneTimeLocal(
        const DateTime &localTime,
        std::function<void(AlarmEventData)> onTrigger = nullptr);
    uint32_t addDaily(
        uint8_t hour24,
        uint8_t minute,
        std::function<void(AlarmEventData)> onTrigger = nullptr);
    uint32_t addWeekly(
        // Accepts either weekday index (0=Sun..6=Sat) or a single DateHelper weekday bit.
        uint8_t weekday,
        uint8_t hour24,
        uint8_t minute,
        std::function<void(AlarmEventData)> onTrigger = nullptr);
    uint32_t addWeekdayMask(
        // OR of DateHelper weekday bits, e.g. DateHelper::MONDAY | DateHelper::WEDNESDAY.
        uint8_t weekdayMask,
        uint8_t hour24,
        uint8_t minute,
        std::function<void(AlarmEventData)> onTrigger = nullptr);
    uint32_t addInterval(
        uint32_t intervalSeconds,
        bool fireImmediately = false,
        std::function<void(AlarmEventData)> onTrigger = nullptr);

    void update();
    uint32_t secondsUntilNextAlarm();
    size_t getAlarmDefinitionsSortedByNextTrigger(AlarmDefinitionWithSchedule *outAlarms, size_t maxOut);

private:
    static constexpr size_t kMaxAlarms = 24;

    AlarmDefinition *findAlarm(uint32_t alarmId);
    uint32_t allocateAlarm(RepeatType repeatType, std::function<void(AlarmEventData)> onTrigger);
    static bool normalizeWeekday(uint8_t weekdayInput, uint8_t &weekdayIndexOut);
    uint32_t secondsUntilAlarm(const AlarmDefinition &alarm, const DateTime &localNow, const DateTime &utcNow);
    bool shouldTrigger(const AlarmDefinition &alarm, const DateTime &localNow, const DateTime &utcNow, uint32_t minuteKey);
    void triggerAlarm(AlarmDefinition &alarm, const DateTime &localNow, const DateTime &utcNow, uint32_t minuteKey);

    RtcManager &rtc_;
    AlarmDefinition alarms_[kMaxAlarms];
    size_t alarmCount_ = 0;
    uint32_t nextAlarmId_ = 1;
    std::function<void(AlarmEventData)> defaultCallback_ = nullptr;
};

#endif  // FIRMWORK_RTCALARMSCHEDULER_H
