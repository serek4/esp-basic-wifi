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

#if defined(ARDUINO_ARCH_ESP8266)
/**
* @brief Convert WiFiDisconnectReason to a string.
* @param reason Error code to be converted.
* @return A string representation of the error code.
* @note taken from esp32 core
*/
const char* BasicWiFi::disconnectReasonName(WiFiDisconnectReason reason) {
	// clang-format off
	switch (reason) {
		case WIFI_DISCONNECT_REASON_UNSPECIFIED:                 return "UNSPECIFIED";
		case WIFI_DISCONNECT_REASON_AUTH_EXPIRE:                 return "AUTH_EXPIRE";
		case WIFI_DISCONNECT_REASON_AUTH_LEAVE:                  return "AUTH_LEAVE";
		case WIFI_DISCONNECT_REASON_ASSOC_EXPIRE:                return "ASSOC_EXPIRE";
		case WIFI_DISCONNECT_REASON_ASSOC_TOOMANY:               return "ASSOC_TOOMANY";
		case WIFI_DISCONNECT_REASON_NOT_AUTHED:                  return "NOT_AUTHED";
		case WIFI_DISCONNECT_REASON_NOT_ASSOCED:                 return "NOT_ASSOCED";
		case WIFI_DISCONNECT_REASON_ASSOC_LEAVE:                 return "ASSOC_LEAVE";
		case WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED:            return "ASSOC_NOT_AUTHED";
		case WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD:         return "DISASSOC_PWRCAP_BAD";
		case WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD:        return "DISASSOC_SUPCHAN_BAD";
		case WIFI_DISCONNECT_REASON_IE_INVALID:                  return "IE_INVALID";
		case WIFI_DISCONNECT_REASON_MIC_FAILURE:                 return "MIC_FAILURE";
		case WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT:      return "4WAY_HANDSHAKE_TIMEOUT";
		case WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT:    return "GROUP_KEY_UPDATE_TIMEOUT";
		case WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS:          return "IE_IN_4WAY_DIFFERS";
		case WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID:        return "GROUP_CIPHER_INVALID";
		case WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID:     return "PAIRWISE_CIPHER_INVALID";
		case WIFI_DISCONNECT_REASON_AKMP_INVALID:                return "AKMP_INVALID";
		case WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION:       return "UNSUPP_RSN_IE_VERSION";
		case WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP:          return "INVALID_RSN_IE_CAP";
		case WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED:          return "802_1X_AUTH_FAILED";
		case WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED:       return "CIPHER_SUITE_REJECTED";
		case WIFI_DISCONNECT_REASON_BEACON_TIMEOUT:              return "BEACON_TIMEOUT";
		case WIFI_DISCONNECT_REASON_NO_AP_FOUND:                 return "NO_AP_FOUND";
		case WIFI_DISCONNECT_REASON_AUTH_FAIL:                   return "AUTH_FAIL";
		case WIFI_DISCONNECT_REASON_ASSOC_FAIL:                  return "ASSOC_FAIL";
		case WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT:           return "HANDSHAKE_TIMEOUT";
		default:                                                 return "UNKNOWN";
	}
	// clang-format on
}
#endif
