#include "esp-basic-wifi.hpp"
#include "enum-names.hpp"

#if defined(ARDUINO_ARCH_ESP8266)
WiFiEventHandler _WiFiConnectedHandler, _WiFiGotIpHandler, _WiFiDisconnectedHandler;
#endif
Ticker _wifiReconnectTimer;
Ticker _wifiDisconnectDelay;

BasicWiFi::BasicWiFi(const char* ssid, const char* pass)
    : _mode(DEFAULT_WIFI_MODE)
    , _staticIP(false)
    , _autoReconnectDelay(AUTO_RECONNECT_DELAY)
    , _status(wifi_idle)
    , _shouldBeConnected(false)
    , _connectingIndicator(nullptr)
    , _logger(nullptr)
    , _ssid(ssid)
    , _pass(pass) {
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
	_ssid = config.ssid;
	_pass = config.pass;
	_mode = config.mode;
	_staticIP = config.staticIP;
	_IP = config.IP;
	_subnet = config.subnet;
	_gateway = config.gateway;
	_dns1 = config.dns1;
	_dns2 = config.dns2;
}
void BasicWiFi::getConfig(BasicWiFi::Config& config) {
	config.ssid = _ssid;
	config.pass = _pass;
	config.mode = _mode;
	config.staticIP = _staticIP;
	config.IP = _IP;
	config.subnet = _subnet;
	config.gateway = _gateway;
	config.dns1 = _dns1;
	config.dns2 = _dns2;
}
BasicWiFi::Config BasicWiFi::getConfig() {
	BasicWiFi::Config config;
	config.ssid = _ssid;
	config.pass = _pass;
	config.mode = _mode;
	config.staticIP = _staticIP;
	config.IP = _IP;
	config.subnet = _subnet;
	config.gateway = _gateway;
	config.dns1 = _dns1;
	config.dns2 = _dns2;
	return config;
}
void BasicWiFi::addLogger(void (*logger)(String logLevel, String msg)) {
	_logger = logger;
}
void BasicWiFi::setMode(WiFiMode_t mode) {
	_mode = mode;
}
void BasicWiFi::setStaticIP(IPAddress IP, IPAddress subnet, IPAddress gateway, IPAddress dns1, IPAddress dns2) {
	_staticIP = true;
	_IP = IP;
	_subnet = subnet;
	_gateway = gateway;
	_dns1 = dns1;
	_dns2 = dns2;
}
void BasicWiFi::setStaticIP(const char* IP, const char* subnet, const char* gateway, const char* dns1, const char* dns2) {
	_staticIP = true;
	_IP.fromString(IP);
	_subnet.fromString(subnet);
	_gateway.fromString(gateway);
	_dns1.fromString(dns1);
	_dns2.fromString(dns2);
}
void BasicWiFi::setup() {
	WiFi.persistent(false);
	WiFi.setAutoReconnect(false);
	if (_staticIP) {
		WiFi.config(_IP, _gateway, _subnet, _dns1, _dns2);
	}
	WiFi.mode(_mode);
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
	if (_logger != nullptr) { (*_logger)("wifi", "connecting WiFi"); }
#ifdef ARDUINO_ARCH_ESP32
	WiFi.bandwidth(WIFI_BW_HT20);
#endif
	WiFi.begin(_ssid, _pass);
	_shouldBeConnected = true;
}
void BasicWiFi::reconnect(uint8_t reconnectDelay) {
	disconnect();
	if (_logger != nullptr) { (*_logger)("wifi", "WiFi reconnect in: " + String(reconnectDelay) + "s"); }
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
	if (_logger != nullptr) { (*_logger)("wifi", logMsg); }
	_status = wifi_connected;
	for (const auto& handler : _onConnectHandlers) handler(HANDLER_ARGS);
}
void BasicWiFi::_onGotIP(GOT_IP_HANDLER_ARGS) {
	_status = wifi_got_ip;
	_wifiReconnectTimer.detach();
	String logMsg = "got IP [" + (WiFi.localIP()).toString() + "]";
	BASIC_WIFI_PRINTLN(logMsg);
	if (_logger != nullptr) { (*_logger)("wifi", logMsg); }
	for (const auto& handler : _onGotIPHandlers) handler(HANDLER_ARGS);
}
void BasicWiFi::_onDisconnected(DISCONNECTED_HANDLER_ARGS) {
	_status = wifi_disconnected;
	String logMsg = "WiFi disconnected [" + statusName() + "]";
#ifdef ARDUINO_ARCH_ESP32
	logMsg += " reason: " + String(WiFi.disconnectReasonName((wifi_err_reason_t)info.wifi_sta_disconnected.reason));
#elif defined(ARDUINO_ARCH_ESP8266)
	logMsg += " reason: " + String(disconnectReasonName(evt.reason));
#endif
	BASIC_WIFI_PRINTLN(logMsg);
	if (_logger != nullptr) { (*_logger)("wifi", logMsg); }
	if (_shouldBeConnected && !_wifiReconnectTimer.active()) { reconnect(_autoReconnectDelay); }
	for (const auto& handler : _onDisconnectHandlers) handler(HANDLER_ARGS);
}
