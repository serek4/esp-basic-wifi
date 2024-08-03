#pragma once

#include "esp-basic-wifi.hpp"

String BasicWiFi::statusName(wl_status_t status) {
	// clang-format off
	switch (status) {
		case WL_NO_SHIELD:          return "NO_SHIELD";
#if defined(ARDUINO_ARCH_ESP32)
		case WL_STOPPED:            return "STOPPED";
#endif
		case WL_IDLE_STATUS:        return "IDLE_STATUS";
		case WL_NO_SSID_AVAIL:      return "NO_SSID_AVAIL";
		case WL_SCAN_COMPLETED:     return "SCAN_COMPLETED";
		case WL_CONNECTED:          return "CONNECTED";
		case WL_CONNECT_FAILED:     return "CONNECT_FAILED";
		case WL_CONNECTION_LOST:    return "CONNECTION_LOST";
#if defined(ARDUINO_ARCH_ESP8266)
		case WL_WRONG_PASSWORD:     return "WRONG_PASSWORD";
#endif
		case WL_DISCONNECTED:       return "DISCONNECTED";
		default:                    return "UNKNOWN";
	}
	// clang-format on
}
