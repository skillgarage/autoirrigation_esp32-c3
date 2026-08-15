#include "settings_storage.h"

#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "STORAGE";
#define SOIL_THRESHOLD_KEY "soil_threshold"
static nvs_handle_t settings_handle;

void settings_storage_init(void){
    ESP_ERROR_CHECK(nvs_open("settings", NVS_READWRITE, &settings_handle));
    ESP_LOGI(TAG, "NVS namespace opened.");
}

int settings_storage_load_threshold(int default_threshold){
    int32_t stored_threshold;

    esp_err_t err = nvs_get_i32(settings_handle, SOIL_THRESHOLD_KEY, &stored_threshold);
    if(err == ESP_ERR_NVS_NOT_FOUND){
        ESP_LOGI(TAG, "Threshold not found, using default value.");
        return default_threshold;
    }

    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "Threshold loaded from NVS: %d", (int)stored_threshold);
    return (int)stored_threshold;
}

void settings_threshold_save_threshold(int threshold){
    ESP_ERROR_CHECK(nvs_set_i32(settings_handle, SOIL_THRESHOLD_KEY, (int32_t)threshold));
    ESP_ERROR_CHECK(nvs_commit(settings_handle));
    ESP_LOGI(TAG, "Threshold saved to NVS %d", threshold);
}
