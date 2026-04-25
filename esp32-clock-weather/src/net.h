#pragma once

#include <Arduino.h>

#include "config.h"

namespace Net {

bool connectWifi(const AppConfig& cfg, uint32_t timeoutMs = 30000);
bool isConnected();

// Configures SNTP with the timezone and NTP server from cfg, then blocks until
// the first sync arrives (or timeout).
bool syncTime(const AppConfig& cfg, uint32_t timeoutMs = 15000);

}  // namespace Net
