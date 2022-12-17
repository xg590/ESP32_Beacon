#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "string.h"
#include "esp_log.h"
#define SSID_LENGTH_OFFSET 37 // 37th element in frame stores the SSID length
static const char *TAG = "[LOG] ";

uint8_t frame[] = {
    // MAC Header (0~23)
    0x80, 0x00,                         // Frame Control
    0x00, 0x00,                         // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // DA
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, // SA
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, // BSS ID
    0x00, 0x00,                         // Sequence Number and Fragment Number
    // Frame Body - Mandatory
    // Fixed Parameters (24~35)
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // Timestamp
    0x64, 0x00,                                     // Beacon Interval (102.4ms)
    0x31, 0x00,                                     // Capatibilities Info
    // Frame Body - Optional
    // First Tagged Param (36~) (see Table 4-7. Information elements of Ref 1)
    0x00,                               // Element ID = 0, which means SSID.
    0x20, // This is the 37th element   // SSID Length (2~32): 32 bytes
    0x20, 0x20, 0x20, 0x20,             // ASCII: 0x20 is the char SPACE
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
};

const char *SSIDs[] = {
    "01 世界杯",
    "02 World Cup",
    "03 45678901234567890123456789012",
    "04 Max length is 32",
    "05 This SSID is too long and will be truncated"
};

void beacon_task(void *_) {
    int num_ssids = sizeof(SSIDs) / sizeof(char *);
    for (;;) {
        for (int i = 0; i < num_ssids; i++) {
            vTaskDelay(100 / num_ssids / portTICK_PERIOD_MS);
            size_t len_ssid = strlen(SSIDs[i]);
            if (len_ssid > 32) len_ssid = 32;
            // Update the the SSID part of beacon frame (begins from the 38th element)
            memcpy(&frame[SSID_LENGTH_OFFSET + 1], SSIDs[i], len_ssid);
            frame[SSID_LENGTH_OFFSET] = len_ssid;   // New SSID, New Length
            esp_wifi_80211_tx(WIFI_IF_AP, frame, SSID_LENGTH_OFFSET + 1 + len_ssid, true);
            ESP_LOGI(TAG, "SSID: %s, Length: %d", SSIDs[i], len_ssid);
        }
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    wifi_config_t ap_config = {
        .ap = {
            .ssid_hidden = true,
            .beacon_interval = 10000
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    xTaskCreate(&beacon_task, "beacon_task", 2048, NULL, 5, NULL);
}