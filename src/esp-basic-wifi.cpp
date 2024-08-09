#include "esp-basic-wifi.hpp"
#include "enum-names.hpp"

#if defined(ARDUINO_ARCH_ESP8266)
WiFiEventHandler _WiFiConnectedHandler, _WiFiGotIpHandler, _WiFiDisconnectedHandler;
#endif
Ticker _wifiReconnectTimer;
Ticker _wifiDisconnectDelay;

BasicWiFi::Config::Config()
    : mode(DEFAULT_WIFI_MODE)
    , ssid("")
    , pass("")
    , ip(NULL_IP_ADDR)
    , subnet(NULL_IP_ADDR)
    , gateway(NULL_IP_ADDR)
    , dns1(NULL_IP_ADDR)
    , dns2(NULL_IP_ADDR) {
}

BasicWiFi::BasicWiFi(const char* ssid, const char* pass)
    : _staticIP(false)
    , _autoReconnectDelay(AUTO_RECONNECT_DELAY)
    , _status(wifi_idle)
    , _shouldBeConnected(false)
    , _connectingIndicator(nullptr)
    , _logger(nullptr) {
	_config.ssid = ssid;
	_config.pass = pass;
}

void BasicWiFi::onConnected(const OnConnectHandler& handler) {
	_onConnectHandlers.push_back(handler);
}
void BasicWiFi::onGotIP(const OnGotIPHandler& handler) {
	_onGotIPHandlers.push_back(handler);
}
void BasicWiFi::onDisconnected(const OnDisconnectHandler& handler) {
	_onDisconnectHandlers.push_back(handler);
}
void BasicWiFi::setConfig(BasicWiFi::Config config) {
	_config.mode = config.mode;
	_config.ssid = config.ssid;
	_config.pass = config.pass;
	if (config.ip != NULL_IP_ADDR && config.subnet != NULL_IP_ADDR) {
		_staticIP = true;
		_config.ip = config.ip;
		_config.subnet = config.subnet;
		_config.gateway = config.gateway;
		_config.dns1 = config.dns1;
		_config.dns2 = config.dns2;
	}
}
BasicWiFi::Config BasicWiFi::getConfig() {
	return _config;
}
void BasicWiFi::addLogger(void (*logger)(uint8_t logLevel, String origin, String msg)) {
	_logger = logger;
}
void BasicWiFi::setMode(WiFiMode_t mode) {
	_config.mode = mode;
}
void BasicWiFi::setStaticIP(IPAddress ip, IPAddress subnet, IPAddress gateway, IPAddress dns1, IPAddress dns2) {
	_staticIP = true;
	_config.ip = ip;
	_config.subnet = subnet;
	_config.gateway = gateway;
	_config.dns1 = dns1;
	_config.dns2 = dns2;
}
void BasicWiFi::setStaticIP(const char* ip, const char* subnet, const char* gateway, const char* dns1, const char* dns2) {
	_staticIP = true;
	_config.ip.fromString(ip);
	_config.subnet.fromString(subnet);
	_config.gateway.fromString(gateway);
	_config.dns1.fromString(dns1);
	_config.dns2.fromString(dns2);
}
void BasicWiFi::setup() {
	WiFi.persistent(false);
	WiFi.setAutoReconnect(false);
	if (_staticIP) {
		WiFi.config(_config.ip, _config.gateway, _config.subnet, _config.dns1, _config.dns2);
	}
	WiFi.mode(_config.mode);
#ifdef ARDUINO_ARCH_ESP32
	// scan all networks and select best RSSI
	WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
	WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
#endif
	connect();
#ifdef ARDUINO_ARCH_ESP32
	WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info) { _onConnected(event, info); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
	WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info) { _onGotIP(event, info); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
	WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info) { _onDisconnected(event, info); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#elif defined(ARDUINO_ARCH_ESP8266)
	_WiFiConnectedHandler = WiFi.onStationModeConnected([&](const WiFiEventStationModeConnected& evt) { _onConnected(evt); });
	_WiFiGotIpHandler = WiFi.onStationModeGotIP([&](const WiFiEventStationModeGotIP& evt) { _onGotIP(evt); });
	_WiFiDisconnectedHandler = WiFi.onStationModeDisconnected([&](const WiFiEventStationModeDisconnected& evt) { _onDisconnected(evt); });
#endif
}
void BasicWiFi::setWaitingFunction(void (*connectingIndicator)(u_long onTime, u_long offTime)) {
	_connectingIndicator = connectingIndicator;
}
int8_t BasicWiFi::waitForConnection(int waitTime) {
	BASIC_WIFI_PRINT("Waiting for WiFi connection");
	u_long startWaitingAt = millis();
	while (_status < wifi_got_ip) {
		BASIC_WIFI_PRINT(".");
		if (_connectingIndicator == nullptr) {
			delay(WIFI_NO_BLINK);
		} else {
			(*_connectingIndicator)(WIFI_BLINK_ON, WIFI_BLINK_OFF);
		}
		if (millis() - startWaitingAt > waitTime * 1000) {
			BASIC_WIFI_PRINTLN("Can't connect to WiFi!");
			return wifi_connection_fail;
		}
	}
	return _status;
}
void BasicWiFi::connect() {
	_log("connecting WiFi", BasicLogs::_info_);
#ifdef ARDUINO_ARCH_ESP32
	WiFi.bandwidth(WIFI_BW_HT20);
#endif
	WiFi.begin(_config.ssid, _config.pass);
	_shouldBeConnected = true;
}
void BasicWiFi::reconnect(uint8_t reconnectDelay) {
	disconnect();
	_log("WiFi reconnect in: " + String(reconnectDelay) + "s", BasicLogs::_info_);
	_wifiReconnectTimer.attach(reconnectDelay, [&]() {
		BASIC_WIFI_PRINTLN("reconnecting");
		connect();
	});
}
void BasicWiFi::disconnect() {
	_wifiReconnectTimer.detach();
	if (_status >= wifi_connected) {
		_wifiDisconnectDelay.once(DISCONNECT_DELAY, []() {
			BASIC_WIFI_PRINTLN("disconnecting");
			WiFi.disconnect();
		});
	} else {
		WiFi.disconnect();
	}
	_shouldBeConnected = false;
}

void BasicWiFi::setAutoReconnectDelay(uint16_t delay) {
	_autoReconnectDelay = delay;
}

void BasicWiFi::addAccessPoint(const AccessPoint& accessPoint) {
	_accessPointsMap.insert(accessPoint);
}
void BasicWiFi::addAccessPoints(const AccessPoints& accessPoints) {
	for (auto const& point : accessPoints) {
		_accessPointsMap.insert(point);
	}
}
void BasicWiFi::setAccessPoints(const AccessPoints& accessPoints) {
	_accessPointsMap = accessPoints;
}
String BasicWiFi::accessPointName(const String& bssidStr) {
	if (_accessPointsMap.count(bssidStr) > 0) {
		return _accessPointsMap.at(bssidStr);
	}
	return bssidStr;
}

bool BasicWiFi::checkDNS(const char* hostname) {
	IPAddress buffer;
	BASIC_WIFI_PRINT("checking DNS server");
	int retry = 0;
	while (WiFi.hostByName(hostname, buffer) == 0) {
		BASIC_WIFI_PRINT(".");
		delay(DNS_CHECK_RETRY_DELAY);
		retry++;
		if (retry > DNS_CHECK_RETRY) {
			BASIC_WIFI_PRINTLN("DNS does not work!");
			return false;
		}
	}
	BASIC_WIFI_PRINTLN(" OK!");
	return true;
}
void BasicWiFi::_onConnected(CONNECTED_HANDLER_ARGS) {
	String logMsg = "WiFi connected to: " + WiFi.SSID() + " [AP: " + accessPointName() + "]";
	BASIC_WIFI_PRINTLN(logMsg);
	_log(logMsg, BasicLogs::_info_);
	_status = wifi_connected;
	for (const auto& handler : _onConnectHandlers) handler(HANDLER_ARGS);
}
void BasicWiFi::_onGotIP(GOT_IP_HANDLER_ARGS) {
	_status = wifi_got_ip;
	_wifiReconnectTimer.detach();
	String logMsg = "got IP [" + (WiFi.localIP()).toString() + "]";
	BASIC_WIFI_PRINTLN(logMsg);
	_log(logMsg, BasicLogs::_info_);
	for (const auto& handler : _onGotIPHandlers) handler(HANDLER_ARGS);
}
void BasicWiFi::_onDisconnected(DISCONNECTED_HANDLER_ARGS) {
	_status = wifi_disconnected;
	String logMsg = "WiFi disconnected [" + statusName() + "]";
#ifdef ARDUINO_ARCH_ESP32
	logMsg += " reason: " + String(WiFi.disconnectReasonName((wifi_err_reason_t)info.wifi_sta_disconnected.reason));
	logMsg += " " + String(info.wifi_sta_disconnected.reason);
#elif defined(ARDUINO_ARCH_ESP8266)
	logMsg += " reason: " + String(disconnectReasonName(evt.reason));
	logMsg += " " + String(evt.reason);
#endif
	BASIC_WIFI_PRINTLN(logMsg);
	_log(logMsg, BasicLogs::_info_);
	if (_shouldBeConnected && !_wifiReconnectTimer.active()) { reconnect(_autoReconnectDelay); }
	for (const auto& handler : _onDisconnectHandlers) handler(HANDLER_ARGS);
}

void BasicWiFi::_log(String message, uint8_t logLevel) {
	if (_logger != nullptr) { (*_logger)(logLevel, "wifi", message); }
}
