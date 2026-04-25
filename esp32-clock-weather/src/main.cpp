// ELECROW ESP32 Rotary Display 1.28" — Clock + Weather
//
// Boot sequence:
//   1. Mount LittleFS, parse /config.json.
//   2. Bring up display, show progress.
//   3. Connect WiFi, sync NTP with configured timezone.
//   4. Fetch initial weather from Open-Meteo.
//   5. Loop: every 1s redraw the face; every N min refresh weather.

#include <Arduino.h>
#include <time.h>

#include "config.h"
#include "net.h"
#include "ui.h"
#include "weather.h"

namespace {

AppConfig    gCfg;
WeatherData  gWeather;
uint32_t     gLastWeatherFetchMs = 0;
bool         gFirstWeatherDone   = false;
int          gLastSecond         = -1;

void refreshWeatherIfDue(bool force = false) {
    const uint32_t intervalMs = gCfg.weatherRefreshMinutes * 60UL * 1000UL;
    const uint32_t now = millis();
    if (!force && gFirstWeatherDone && (now - gLastWeatherFetchMs) < intervalMs) return;
    if (!Net::isConnected()) return;

    WeatherData w;
    if (Weather::fetch(gCfg, w)) {
        gWeather = w;
        gFirstWeatherDone   = true;
        gLastWeatherFetchMs = now;
    } else {
        // Keep stale data on failure; retry in 60s.
        gLastWeatherFetchMs = now - intervalMs + 60UL * 1000UL;
    }
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1500);   // give USB-CDC time to enumerate before first prints
    Serial.println();
    Serial.println(F("=== ESP32-S3 1.28\" rotary clock+weather ==="));
    Serial.printf("[boot] chip: %s rev %d  cores: %d  PSRAM: %u\n",
                  ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores(),
                  (unsigned)ESP.getPsramSize());
    Serial.printf("[boot] free heap: %u  free PSRAM: %u\n",
                  (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());

    Serial.println(F("[boot] loading config.json"));
    const bool cfgOk = Config::load(gCfg);
    Serial.printf("[boot] config: %s\n", cfgOk ? "ok" : "FAILED");

    Serial.println(F("[boot] UI::begin"));
    UI::begin(gCfg.twentyFourHour, gCfg.showSeconds);
    Serial.println(F("[boot] UI ready"));

    if (!cfgOk) {
        UI::showStatus("Config error", "check /config.json");
        return;
    }

    UI::showStatus("Connecting WiFi", gCfg.wifiSsid.c_str());
    Serial.printf("[boot] WiFi connecting to '%s'\n", gCfg.wifiSsid.c_str());
    if (!Net::connectWifi(gCfg)) {
        Serial.println(F("[boot] WiFi FAILED, rebooting in 10s"));
        UI::showStatus("WiFi failed", "rebooting in 10s");
        delay(10000);
        ESP.restart();
    }

    UI::showStatus("Syncing time", gCfg.timezone.c_str());
    Serial.printf("[boot] NTP sync via %s (tz %s)\n",
                  gCfg.ntpServer.c_str(), gCfg.timezone.c_str());
    Net::syncTime(gCfg);

    UI::showStatus("Fetching weather", "");
    Serial.println(F("[boot] fetching weather"));
    refreshWeatherIfDue(/*force=*/true);
    Serial.println(F("[boot] setup complete"));
}

void loop() {
    // ESP32 auto-reconnects WiFi in the background; we just observe.
    refreshWeatherIfDue();

    struct tm now;
    if (getLocalTime(&now, 50)) {
        if (now.tm_sec != gLastSecond) {
            gLastSecond = now.tm_sec;
            UI::render(now, gWeather);
        }
    }

    delay(40);   // ~25 Hz check; render at 1 Hz
}
