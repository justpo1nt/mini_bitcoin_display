#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h> 

#include <U8g2lib.h>

#include <HTTPClient.h>

#include <ArduinoJson.h>

#include <ArduinoOTA.h>

const uint8_t bitcoin_bits[] = {
  0x00, 0xF0, 0x07, 0x00, 0x00, 0x0E, 0x38, 0x00, 0x80, 0x01, 0xC0, 0x00, 
  0x40, 0x00, 0x00, 0x01, 0x20, 0x00, 0x00, 0x02, 0x10, 0x00, 0x00, 0x04, 
  0x08, 0x30, 0x03, 0x08, 0x04, 0x30, 0x03, 0x10, 0x04, 0x30, 0x03, 0x10, 
  0x02, 0xFE, 0x0F, 0x20, 0x02, 0xFE, 0x1F, 0x20, 0x02, 0x38, 0x3C, 0x20, 
  0x01, 0x38, 0x3C, 0x40, 0x01, 0x38, 0x1C, 0x40, 0x01, 0xF8, 0x0F, 0x40, 
  0x01, 0xF8, 0x3F, 0x40, 0x01, 0x38, 0x7C, 0x40, 0x01, 0x38, 0x78, 0x40, 
  0x01, 0x38, 0x78, 0x40, 0x02, 0x38, 0x7C, 0x20, 0x02, 0xFE, 0x3F, 0x20, 
  0x02, 0xFE, 0x1F, 0x20, 0x04, 0x30, 0x03, 0x10, 0x04, 0x30, 0x03, 0x10, 
  0x08, 0x30, 0x03, 0x08, 0x10, 0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x02, 
  0x40, 0x00, 0x00, 0x01, 0x80, 0x01, 0xC0, 0x00, 0x00, 0x0E, 0x38, 0x00, 
  0x00, 0xF0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, };

AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifi_manager(&server,&dns);

const char* setup_ssid = "ESP32_BitcoinDisplay";
const char* setup_pass = "1234567890";

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 16, 17);

String api_url = "https://api.coinbase.com/v2/prices/spot?currency=USD";
unsigned long last_update = 0;
const unsigned long update_interval = 60000;

String current_amount = "";
String message = "Hello World";
void configModeCallback(AsyncWiFiManager* myWiFiManager) {
    message = "Config Mode";
}

void setup() {
    Serial.begin(115200);

    u8g2.begin();
    wifi_manager.startConfigPortalModeless(setup_ssid, setup_pass);

    wifi_manager.setAPCallback(configModeCallback);

    ArduinoOTA.begin();
}

void loop() {
    wifi_manager.loop();
    ArduinoOTA.handle();

    if (last_update + update_interval > millis() && last_update > 0) {
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(api_url.c_str());

        int httpResponseCode = http.GET();
        if (httpResponseCode == 200) {
            last_update = millis();

            String content = http.getString();
            DynamicJsonDocument doc(content.length() * 2);

            deserializeJson(doc, content);
            JsonObject data = doc["data"].as<JsonObject>();
            current_amount = data["amount"].as<String>();
        }
    }

    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_ncenB14_tr);
        if (current_amount.length() == 0) {
            u8g2.setFont(u8g2_font_squeezed_b6_tr);
            u8g2.drawStr(4, 4, "SSID: ");
            u8g2.drawStr(4, 12, "PASS: ");
            unsigned int x_ssid = u8g2.getStrWidth("SSID: ");
            unsigned int x_pass = u8g2.getStrWidth("PASS: ");
            u8g2.setFont(u8g2_font_squeezed_r6_tr);
            u8g2.drawStr(4 + x_ssid + 2, 4, setup_ssid);
            u8g2.drawStr(4 + x_pass + 2, 12, setup_pass);
            continue;
        }

        u8g2.drawXBM(4, 16, 32, 32, bitcoin_bits);
        u8g2.drawStr(40, 39, current_amount.c_str());
        u8g2.setFont(u8g2_font_squeezed_b6_tr);
        u8g2.drawStr(124 - u8g2.getStrWidth("by Po1nt"), 58, "by Po1nt");
    } while (u8g2.nextPage());
}