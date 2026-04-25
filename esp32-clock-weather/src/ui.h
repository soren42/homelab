#pragma once

#include <Arduino.h>
#include <time.h>

#include "weather.h"

namespace UI {

void begin(bool twentyFourHour, bool showSeconds);

// Draw a status / boot message centered on screen.
void showStatus(const char* line1, const char* line2 = nullptr);

// Render a single frame: clock face + hands + digital time + weather bg.
void render(const struct tm& localTime, const WeatherData& weather);

}  // namespace UI
