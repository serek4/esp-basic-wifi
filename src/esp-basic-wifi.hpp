#pragma once

#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#elif defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Ticker.h>
#include <esp-basic-logs.hpp>
#include <functional>
#include <list>
#include <map>

// #define BASIC_WIFI_DEBUG
// debug printing macros
// clang-format off
#ifdef BASIC_WIFI_DEBUG
#define DEBUG_PRINTER Serial
#define BASIC_WIFI_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
#define BASIC_WIFI_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#define BASIC_WIFI_PRINTF(...) { DEBUG_PRINTER.printf(__VA_ARGS__); }
#else
#define BASIC_WIFI_PRINT(...) {}
#define BASIC_WIFI_PRINTLN(...) {}
#define BASIC_WIFI_PRINTF(...) {}
#endif
// clang-format on

#define WIFI_BLINK_ON 200
#define WIFI_BLINK_OFF 300
#define WIFI_NO_BLINK WIFI_BLINK_ON + WIFI_BLINK_OFF

#define DNS_CHECK_RETRY 3
#define DNS_CHECK_RETRY_DELAY 100

#define DEFAULT_RECONNECT_DELAY 5
#define AUTO_RECONNECT_DELAY 30
#define DISCONNECT_DELAY 0.1
#define DEFAULT_WIFI_MODE WIFI_STA
#define DEFAULT_CONNECTION_WAIT_TIME 10

#if defined(ARDUINO_ARCH_ESP32)
#define CONNECTED_HANDLER_ARGS const WiFiEvent_t event, WiFiEventInfo_t info
#define GOT_IP_HANDLER_ARGS const WiFiEvent_t event, WiFiEventInfo_t info
#define DISCONNECTED_HANDLER_ARGS const WiFiEvent_t event, WiFiEventInfo_t info
#define HANDLER_ARGS event, info
#define NULL_IP_ADDR INADDR_NONE
#elif defined(ARDUINO_ARCH_ESP8266)
#define CONNECTED_HANDLER_ARGS const WiFiEventStationModeConnected& evt
#define GOT_IP_HANDLER_ARGS const WiFiEventStationModeGotIP& evt
#define DISCONNECTED_HANDLER_ARGS const WiFiEventStationModeDisconnected& evt
#define HANDLER_ARGS evt
#define NULL_IP_ADDR INADDR_ANY
#endif

using AccessPoint = std::pair<String, String>;
using AccessPoints = std::map<String, String>;

class BasicWiFi {
  public:
	typedef std::function<void(CONNECTED_HANDLER_ARGS)> OnConnectHandler;
	typedef std::function<void(GOT_IP_HANDLER_ARGS)> OnGotIPHandler;
	typedef std::function<void(DISCONNECTED_HANDLER_ARGS)> OnDisconnectHandler;
	enum Status {
		wifi_idle = -2,
		wifi_connection_fail,
		wifi_disconnected,
		wifi_connected,
		wifi_got_ip,
	};
	struct Config {
		Config();
		WiFiMode_t mode;
		String ssid;
		String pass;
		IPAddress ip;
		IPAddress subnet;
		IPAddress gateway;
		IPAddress dns1;
		IPAddress dns2;
	};

	BasicWiFi(const char* ssid, const char* pass);

	void setConfig(BasicWiFi::Config config);
	Config getConfig();
	void addLogger(void (*logger)(uint8_t logLevel, String origin, String msg));
	void setMode(WiFiMode_t mode);
	void setStaticIP(IPAddress IP, IPAddress subnet, IPAddress gateway, IPAddress dns1, IPAddress dns2);
	void setStaticIP(const char* IP, const char* subnet, const char* gateway, const char* dns1, const char* dns2);
	void setup();
	void setWaitingFunction(void (*connectingIndicator)(u_long onTime, u_long offTime));
	int8_t waitForConnection(int waitTime = DEFAULT_CONNECTION_WAIT_TIME);
	bool checkDNS(const char* hostname = "google.com");
	void onConnected(const OnConnectHandler& handler);
	void onGotIP(const OnGotIPHandler& handler);
	void onDisconnected(const OnDisconnectHandler& handler);
	void connect();
	void reconnect(uint8_t reconnectDelay = DEFAULT_RECONNECT_DELAY);
	void disconnect();
	void setAutoReconnectDelay(uint16_t delay);
	void addAccessPoint(const AccessPoint& accessPoint);
	void addAccessPoints(const AccessPoints& accessPoints);
	void setAccessPoints(const AccessPoints& accessPoints);
	String accessPointName(const String& bssidStr = WiFi.BSSIDstr());
	String statusName(wl_status_t status = WiFi.status());
#if defined(ARDUINO_ARCH_ESP8266)
	const char* disconnectReasonName(WiFiDisconnectReason reason);
#endif

  private:
	Config _config;

	bool _staticIP;
	void (*_connectingIndicator)(u_long onTime, u_long offTime);
	void (*_logger)(uint8_t logLevel, String origin, String msg);

	AccessPoints _accessPointsMap;    // BSSIDstr, access point name
	uint16_t _autoReconnectDelay;
	int8_t _status;
	bool _shouldBeConnected;
	std::list<OnConnectHandler> _onConnectHandlers;
	std::list<OnGotIPHandler> _onGotIPHandlers;
	std::list<OnDisconnectHandler> _onDisconnectHandlers;

	void _onConnected(CONNECTED_HANDLER_ARGS);
	void _onGotIP(GOT_IP_HANDLER_ARGS);
	void _onDisconnected(DISCONNECTED_HANDLER_ARGS);

	void _log(String message, uint8_t logLevel = BasicLogs::_none_);
};
