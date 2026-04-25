#include "config.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace {

struct TzEntry {
    const char* iana;
    const char* posix;
};

// Common IANA -> POSIX TZ mappings. Add more as needed for your locale.
constexpr TzEntry kTzTable[] = {
    {"UTC",                  "UTC0"},
    {"America/New_York",     "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Chicago",      "CST6CDT,M3.2.0,M11.1.0"},
    {"America/Denver",       "MST7MDT,M3.2.0,M11.1.0"},
    {"America/Phoenix",      "MST7"},
    {"America/Los_Angeles",  "PST8PDT,M3.2.0,M11.1.0"},
    {"America/Anchorage",    "AKST9AKDT,M3.2.0,M11.1.0"},
    {"America/Honolulu",     "HST10"},
    {"Europe/London",        "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Europe/Paris",         "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Berlin",        "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Madrid",        "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Rome",          "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Athens",        "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Asia/Tokyo",           "JST-9"},
    {"Asia/Shanghai",        "CST-8"},
    {"Asia/Singapore",       "SGT-8"},
    {"Asia/Dubai",           "GST-4"},
    {"Asia/Kolkata",         "IST-5:30"},
    {"Australia/Sydney",     "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"Pacific/Auckland",     "NZST-12NZDT,M9.5.0,M4.1.0/3"},
};

}  // namespace

namespace Config {

const char* posixTzFor(const String& iana) {
    for (const auto& e : kTzTable) {
        if (iana == e.iana) return e.posix;
    }
    return "UTC0";
}

bool load(AppConfig& out) {
    if (!LittleFS.begin(true)) {
        log_e("LittleFS mount failed");
        return false;
    }

    File f = LittleFS.open("/config.json", "r");
    if (!f) {
        log_e("/config.json not found on LittleFS");
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        log_e("config.json parse error: %s", err.c_str());
        return false;
    }

    out.wifiSsid     = doc["wifi"]["ssid"].as<const char*>() ?: "";
    out.wifiPassword = doc["wifi"]["password"].as<const char*>() ?: "";

    out.timezone  = doc["time"]["timezone"].as<const char*>()   ?: "UTC";
    out.ntpServer = doc["time"]["ntp_server"].as<const char*>() ?: "pool.ntp.org";

    out.latitude  = doc["weather"]["latitude"]  | 0.0f;
    out.longitude = doc["weather"]["longitude"] | 0.0f;
    out.units     = doc["weather"]["units"].as<const char*>() ?: "imperial";
    out.weatherRefreshMinutes = doc["weather"]["refresh_minutes"] | 15;

    out.showSeconds    = doc["display"]["show_seconds"] | true;
    out.twentyFourHour = doc["display"]["24_hour"]      | false;

    return true;
}

}  // namespace Config
