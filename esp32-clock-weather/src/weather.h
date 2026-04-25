#pragma once

#include <Arduino.h>

#include "config.h"

// Coarse weather categories for UI background selection.
enum class WeatherKind : uint8_t {
    Unknown = 0,
    Clear,        // sunny / clear
    PartlyCloudy,
    Cloudy,       // overcast
    Fog,
    Rain,
    Snow,
    Storm,        // thunderstorm
};

struct WeatherData {
    bool       valid = false;
    bool       isDay = true;
    float      temperature = 0.0f;   // in configured units
    float      windSpeed   = 0.0f;   // in configured units
    int        wmoCode     = -1;     // raw WMO weather code
    WeatherKind kind       = WeatherKind::Unknown;
    char       unitsTemp[4] = "F";   // "F" or "C"
};

namespace Weather {

bool fetch(const AppConfig& cfg, WeatherData& out);
WeatherKind classify(int wmoCode);
const char* kindName(WeatherKind k);

}  // namespace Weather
