#include <esp-basic-wifi.h>

#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"

BasicWiFi wifi(WIFI_SSID, WIFI_PASS);

void setup() {
	Serial.begin(115200);
	Serial.println();
	wifi.onConnected(handleWiFiConnected);
	wifi.onGotIP(handleWiFiGotIP);
	wifi.onDisconnected(handleWiFiDisconnected);
	wifi.setup();
	if (wifi.waitForConnection() == BasicWiFi::connected) {
	}
	Serial.println("setup done!");
}

void loop() {
	delay(10);
}

void handleWiFiConnected(CONNECTED_HANDLER_ARGS) {
	Serial.println("User handler for WIFI onConnected");
}
void handleWiFiGotIP(GOT_IP_HANDLER_ARGS) {
	Serial.println("User handler for WIFI onGotIP");
}
void handleWiFiDisconnected(DISCONNECTED_HANDLER_ARGS) {
	Serial.println("User handler for WIFI onDisconnected");
}
