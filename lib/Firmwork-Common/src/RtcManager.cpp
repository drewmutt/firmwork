#include <RtcManager.h>

bool Ds3231RtcProvider::begin() {
    return rtc_.begin();
}

bool Ds3231RtcProvider::lostPower() {
    return rtc_.lostPower();
}

void Ds3231RtcProvider::adjust(const DateTime &utcTime) {
    rtc_.adjust(utcTime);
}

DateTime Ds3231RtcProvider::now() {
    return rtc_.now();
}

bool Ds1307RtcProvider::begin() {
    return rtc_.begin();
}

bool Ds1307RtcProvider::lostPower() {
    return !rtc_.isrunning();
}

void Ds1307RtcProvider::adjust(const DateTime &utcTime) {
    rtc_.adjust(utcTime);
}

DateTime Ds1307RtcProvider::now() {
    return rtc_.now();
}

RtcManager::RtcManager(const Config &config) : config_(config) {}

RtcManager::Config RtcManager::defaultConfig() {
    return {
        RTC_MODEL_DS3231,
        DateHelper::UTCConfig(),
        true,
    };
}

void RtcManager::setModel(Model model) {
    config_.model = model;
    ready_ = false;
}

RtcManager::Model RtcManager::getModel() const {
    return config_.model;
}

void RtcManager::setTimeZone(const DateHelper::TimeZoneConfig &timeZone) {
    config_.timeZone = timeZone;
}

const DateHelper::TimeZoneConfig &RtcManager::getTimeZone() const {
    return config_.timeZone;
}

bool RtcManager::begin() {
    RtcProvider *provider = activeProvider();
    if (provider == nullptr) {
        ready_ = false;
        return false;
    }

    ready_ = provider->begin();
    if (!ready_) {
        return false;
    }

    if (config_.setFromBuildOnPowerLoss && provider->lostPower()) {
        return setFromBuildMacros(true);
    }

    return true;
}

bool RtcManager::isReady() const {
    return ready_;
}

bool RtcManager::lostPower() const {
    const RtcProvider *provider = activeProvider();
    if (!ready_ || provider == nullptr) {
        return false;
    }

    return const_cast<RtcProvider *>(provider)->lostPower();
}

bool RtcManager::setUtcTime(const DateTime &utcTime) {
    RtcProvider *provider = activeProvider();
    if (!ready_ || provider == nullptr) {
        return false;
    }

    provider->adjust(utcTime);
    return true;
}

bool RtcManager::setLocalTime(const DateTime &localTime) {
    return setUtcTime(DateHelper::utcFromLocal(localTime, config_.timeZone));
}

bool RtcManager::setFromBuildTime(
    const __FlashStringHelper *buildDate,
    const __FlashStringHelper *buildTime,
    bool buildTimeIsLocal) {
    if (buildTimeIsLocal) {
        return setLocalTime(DateHelper::localFromBuildTime(buildDate, buildTime));
    }

    return setUtcTime(DateTime(buildDate, buildTime));
}

bool RtcManager::setFromBuildMacros(bool buildTimeIsLocal) {
    return setFromBuildTime(F(__DATE__), F(__TIME__), buildTimeIsLocal);
}

bool RtcManager::setTimeFromBuildUnixTime() {
#ifdef BUILD_UNIX_TIME
    return setUtcTime(DateTime(static_cast<uint32_t>(BUILD_UNIX_TIME)));
#else
    return false;
#endif
}

DateTime RtcManager::nowUtc() {
    RtcProvider *provider = activeProvider();
    if (!ready_ || provider == nullptr) {
        return DateTime((uint32_t)0);
    }

    return provider->now();
}

DateTime RtcManager::nowLocal() {
    return DateHelper::localFromUtc(nowUtc(), config_.timeZone);
}


bool RtcManager::nowUtc(DateTime &outUtc) {
    if (!ready_) {
        return false;
    }

    outUtc = nowUtc();
    return true;
}

bool RtcManager::nowLocal(DateTime &outLocal) {
    if (!ready_) {
        return false;
    }

    outLocal = nowLocal();
    return true;
}

RtcProvider *RtcManager::activeProvider() {
    switch (config_.model) {
        case RTC_MODEL_DS1307:
            return &ds1307Provider_;
        case RTC_MODEL_DS3231:
        default:
            return &ds3231Provider_;
    }
}

const RtcProvider *RtcManager::activeProvider() const {
    switch (config_.model) {
        case RTC_MODEL_DS1307:
            return &ds1307Provider_;
        case RTC_MODEL_DS3231:
        default:
            return &ds3231Provider_;
    }
}
