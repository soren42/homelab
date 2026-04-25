#include "weather.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

namespace Weather {

WeatherKind classify(int code) {
    // WMO weather interpretation codes used by Open-Meteo.
    if (code < 0) return WeatherKind::Unknown;
    if (code == 0) return WeatherKind::Clear;
    if (code == 1 || code == 2) return WeatherKind::PartlyCloudy;
    if (code == 3) return WeatherKind::Cloudy;
    if (code == 45 || code == 48) return WeatherKind::Fog;
    if ((code >= 51 && code <= 67) ||
        (code >= 80 && code <= 82)) return WeatherKind::Rain;
    if ((code >= 71 && code <= 77) ||
        (code == 85 || code == 86)) return WeatherKind::Snow;
    if (code >= 95 && code <= 99) return WeatherKind::Storm;
    return WeatherKind::Unknown;
}

const char* kindName(WeatherKind k) {
    switch (k) {
        case WeatherKind::Clear:        return "Clear";
        case WeatherKind::PartlyCloudy: return "Partly Cloudy";
        case WeatherKind::Cloudy:       return "Cloudy";
        case WeatherKind::Fog:          return "Fog";
        case WeatherKind::Rain:         return "Rain";
        case WeatherKind::Snow:         return "Snow";
        case WeatherKind::Storm:        return "Storm";
        default:                        return "—";
    }
}

bool fetch(const AppConfig& cfg, WeatherData& out) {
    const bool imperial = cfg.units.equalsIgnoreCase("imperial");
    const char* tUnit   = imperial ? "fahrenheit"   : "celsius";
    const char* wUnit   = imperial ? "mph"          : "kmh";

    String url = "https://api.open-meteo.com/v1/forecast?latitude=";
    url += String(cfg.latitude, 4);
    url += "&longitude=";
    url += String(cfg.longitude, 4);
    url += "&current=temperature_2m,weather_code,wind_speed_10m,is_day";
    url += "&temperature_unit=";
    url += tUnit;
    url += "&wind_speed_unit=";
    url += wUnit;
    url += "&timezone=auto";

    WiFiClientSecure client;
    client.setInsecure();   // Open-Meteo TLS; no cert pinning for this demo.

    HTTPClient http;
    http.setTimeout(8000);
    if (!http.begin(client, url)) {
        log_e("HTTP begin failed");
        return false;
    }

    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        log_e("Weather HTTP %d", code);
        http.end();
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();
    if (err) {
        log_e("Weather JSON parse: %s", err.c_str());
        return false;
    }

    JsonObjectConst cur = doc["current"];
    if (cur.isNull()) {
        log_e("Weather payload missing 'current'");
        return false;
    }

    out.temperature = cur["temperature_2m"] | 0.0f;
    out.windSpeed   = cur["wind_speed_10m"] | 0.0f;
    out.wmoCode     = cur["weather_code"]   | -1;
    out.isDay       = (cur["is_day"] | 1) != 0;
    out.kind        = classify(out.wmoCode);
    strncpy(out.unitsTemp, imperial ? "F" : "C", sizeof(out.unitsTemp));
    out.valid = true;

    log_i("Weather: %.1f°%s code=%d kind=%s day=%d",
          out.temperature, out.unitsTemp, out.wmoCode,
          kindName(out.kind), out.isDay ? 1 : 0);
    return true;
}

}  // namespace Weather
