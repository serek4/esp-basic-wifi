#include "esp-basic-wifi.hpp"

#if defined(ARDUINO_ARCH_ESP8266)
WiFiEventHandler _WiFiConnectedHandler, _WiFiGotIpHandler, _WiFiDisconnectedHandler;
#endif
Ticker _wifiReconnectTimer;
Ticker _wifiDisconnectDelay;
String BasicWiFi::_ssid;
String BasicWiFi::_pass;

BasicWiFi::BasicWiFi(const char* ssid, const char* pass)
    : _mode(DEFAULT_WIFI_MODE)
    , _staticIP(false)
    , _connected(false)
    , _connectingIndicator(nullptr)
    , _logger(nullptr) {
	_ssid = ssid;
	_pass = pass;
}
BasicWiFi::BasicWiFi(const char* ssid, const char* pass, int mode)
    : _mode(static_cast<WiFiMode_t>(mode))
    , _staticIP(false)
    , _connected(false)
    , _connectingIndicator(nullptr)
    , _logger(nullptr) {
	_ssid = ssid;
	_pass = pass;
}
BasicWiFi::BasicWiFi(const char* ssid, const char* pass, int mode, const char* IP, const char* subnet, const char* gateway, const char* dns1, const char* dns2)
    : _mode(static_cast<WiFiMode_t>(mode))
    , _staticIP(true)
    , _connected(false)
    , _connectingIndicator(nullptr)
    , _logger(nullptr) {
	_ssid = ssid;
	_pass = pass;
	(_IP).fromString(IP);
	(_subnet).fromString(subnet);
	(_gateway).fromString(gateway);
	(_dns1).fromString(dns1);
	(_dns2).fromString(dns2);
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
void BasicWiFi::setup() {
	WiFi.setAutoReconnect(false);
	WiFi.disconnect(true, true);
	if (_staticIP) {
		WiFi.config(_IP, _gateway, _subnet, _dns1, _dns2);
	}
	WiFi.mode(_mode);
	WiFi.persistent(false);
#ifdef ARDUINO_ARCH_ESP32
	// scan all networks and select best RSSI
	WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
	WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
#endif
	WiFi.begin(_ssid, _pass);
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
uint8_t BasicWiFi::waitForConnection(int waitTime) {
	BASIC_WIFI_PRINT("Waiting for WiFi connection");
	u_long startWaitingAt = millis();
	while (WiFi.status() != WL_CONNECTED) {
		BASIC_WIFI_PRINT(".");
		if (_connectingIndicator == nullptr) {
			delay(WIFI_NO_BLINK);
		} else {
			(*_connectingIndicator)(WIFI_BLINK_ON, WIFI_BLINK_OFF);
		}
		if (millis() - startWaitingAt > waitTime * 1000) {
			BASIC_WIFI_PRINTLN("Can't connect to WiFi!");
			return connection_fail;
		}
	}
	return _checkConnection();
}
void BasicWiFi::connect() {
	WiFi.begin(_ssid, _pass);
}
void BasicWiFi::reconnect(uint8_t reconnectDelay) {
	disconnect();
	_wifiReconnectTimer.attach(reconnectDelay, []() {
		BASIC_WIFI_PRINTLN("reconnecting");
		connect();
	});
}
void BasicWiFi::disconnect() {
	if (_connected) {
		_connected = false;
		_wifiReconnectTimer.detach();
		_wifiDisconnectDelay.once(DISCONNECT_DELAY, []() {
			BASIC_WIFI_PRINTLN("disconnecting");
			WiFi.disconnect(true);
		});
	} else {
		WiFi.disconnect(true);
	}
}

uint8_t BasicWiFi::_checkConnection() {
	IPAddress buffer;
	BASIC_WIFI_PRINT("checking DNS server");
	int retry = 0;
	while (WiFi.hostByName("google.com", buffer) == 0) {
		BASIC_WIFI_PRINT(".");
		delay(DNS_CHECK_RETRY_DELAY);
		retry++;
		if (retry > DNS_CHECK_RETRY) {
			BASIC_WIFI_PRINTLN("DNS does not work!");
			return dns_fail;
		}
	}
	BASIC_WIFI_PRINTLN(" OK!");
	return connected;
}
void BasicWiFi::_onConnected(CONNECTED_HANDLER_ARGS) {
	BASIC_WIFI_PRINTLN("WiFi connected!\n SSID: " + WiFi.SSID());
	if (_logger != nullptr) { (*_logger)("wifi", "WiFi connected to: " + WiFi.SSID() + " [" + WiFi.BSSIDstr() + "]"); }
	for (const auto& handler : _onConnectHandlers) handler(HANDLER_ARGS);
}
void BasicWiFi::_onGotIP(GOT_IP_HANDLER_ARGS) {
	_connected = true;
	_wifiReconnectTimer.detach();
	BASIC_WIFI_PRINTLN(" IP:   " + WiFi.localIP().toString());
	if (_logger != nullptr) { (*_logger)("wifi", "got IP [" + (WiFi.localIP()).toString() + "]"); }
	for (const auto& handler : _onGotIPHandlers) handler(HANDLER_ARGS);
}
void BasicWiFi::_onDisconnected(DISCONNECTED_HANDLER_ARGS) {
	BASIC_WIFI_PRINTLN("WiFi disconnected");
	if (_logger != nullptr) { (*_logger)("wifi", "WiFi disconnected [" + String(_wifiStatus[WiFi.status()]) + "]"); }
	if (_connected) { reconnect(AUTO_RECONNECT_DELAY); }
	_connected = false;
	for (const auto& handler : _onDisconnectHandlers) handler(HANDLER_ARGS);
}
