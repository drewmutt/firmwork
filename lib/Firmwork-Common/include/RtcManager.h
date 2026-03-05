//
// RTC manager abstraction with model selection and timezone handling.
//

#ifndef FIRMWORK_RTCMANAGER_H
#define FIRMWORK_RTCMANAGER_H

#include <DateHelper.h>
#include <RTClib.h>

class RtcProvider {
public:
    virtual ~RtcProvider() = default;
    virtual bool begin() = 0;
    virtual bool lostPower() = 0;
    virtual void adjust(const DateTime &utcTime) = 0;
    virtual DateTime now() = 0;
};

class Ds3231RtcProvider : public RtcProvider {
public:
    bool begin() override;
    bool lostPower() override;
    void adjust(const DateTime &utcTime) override;
    DateTime now() override;

private:
    RTC_DS3231 rtc_;
};

class Ds1307RtcProvider : public RtcProvider {
public:
    bool begin() override;
    bool lostPower() override;
    void adjust(const DateTime &utcTime) override;
    DateTime now() override;

private:
    RTC_DS1307 rtc_;
};

class RtcManager {
public:
    enum Model {
        RTC_MODEL_DS3231 = 0,
        RTC_MODEL_DS1307 = 1,
    };

    struct Config {
        Model model;
        DateHelper::TimeZoneConfig timeZone;
        bool setFromBuildOnPowerLoss;
    };

    explicit RtcManager(const Config &config = defaultConfig());

    static Config defaultConfig();

    void setModel(Model model);
    Model getModel() const;

    void setTimeZone(const DateHelper::TimeZoneConfig &timeZone);
    const DateHelper::TimeZoneConfig &getTimeZone() const;

    bool begin();
    bool isReady() const;
    bool lostPower() const;

    bool setUtcTime(const DateTime &utcTime);
    bool setLocalTime(const DateTime &localTime);

    bool setFromBuildTime(
        const __FlashStringHelper *buildDate,
        const __FlashStringHelper *buildTime,
        bool buildTimeIsLocal = true);
    bool setFromBuildMacros(bool buildTimeIsLocal = true);
    bool setTimeFromBuildUnixTime();

    DateTime nowUtc();
    DateTime nowLocal();

    bool nowUtc(DateTime &outUtc);
    bool nowLocal(DateTime &outLocal);

private:
    RtcProvider *activeProvider();
    const RtcProvider *activeProvider() const;

    Config config_;
    Ds3231RtcProvider ds3231Provider_;
    Ds1307RtcProvider ds1307Provider_;
    bool ready_ = false;
};

#endif  // FIRMWORK_RTCMANAGER_H
