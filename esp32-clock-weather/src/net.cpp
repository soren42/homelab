#include "net.h"

#include <WiFi.h>
#include <time.h>

namespace Net {

bool connectWifi(const AppConfig& cfg, uint32_t timeoutMs) {
    if (cfg.wifiSsid.isEmpty()) {
        log_e("WiFi SSID is empty in config.json");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPassword.c_str());

    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
        delay(250);
    }
    if (WiFi.status() != WL_CONNECTED) {
        log_e("WiFi connect timeout");
        return false;
    }
    log_i("WiFi connected: %s", WiFi.localIP().toString().c_str());
    return true;
}

bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool syncTime(const AppConfig& cfg, uint32_t timeoutMs) {
    const char* posixTz = Config::posixTzFor(cfg.timezone);
    configTzTime(posixTz, cfg.ntpServer.c_str(), "time.google.com", "time.cloudflare.com");

    const uint32_t start = millis();
    struct tm now {};
    while (millis() - start < timeoutMs) {
        if (getLocalTime(&now, 500) && now.tm_year > (2020 - 1900)) {
            log_i("NTP sync OK: %04d-%02d-%02d %02d:%02d:%02d (%s)",
                  now.tm_year + 1900, now.tm_mon + 1, now.tm_mday,
                  now.tm_hour, now.tm_min, now.tm_sec, cfg.timezone.c_str());
            return true;
        }
    }
    log_e("NTP sync timeout");
    return false;
}

}  // namespace Net
