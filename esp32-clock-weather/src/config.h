#pragma once

#include <Arduino.h>

struct AppConfig {
    String wifiSsid;
    String wifiPassword;

    String timezone     = "UTC";          // IANA name, e.g. "America/New_York"
    String ntpServer    = "pool.ntp.org";

    float  latitude     = 0.0f;
    float  longitude    = 0.0f;
    String units        = "imperial";     // "imperial" or "metric"
    uint32_t weatherRefreshMinutes = 15;

    bool showSeconds    = true;
    bool twentyFourHour = false;
};

namespace Config {
    bool load(AppConfig& out);
    const char* posixTzFor(const String& iana);
}
