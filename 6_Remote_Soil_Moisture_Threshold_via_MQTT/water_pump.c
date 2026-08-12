#include "water_pump.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "Water_Pump";

#define PUMP_GPIO GPIO_NUM_4
#define PUMP_BUTTON_GPIO GPIO_NUM_5

static bool pump_enabled = false;

void water_pump_init(){
    gpio_config_t pump_config = {
        .pin_bit_mask = 1ULL << PUMP_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&pump_config));

    ESP_ERROR_CHECK(gpio_set_level(PUMP_GPIO, 0));
    pump_enabled = false;

    gpio_config_t button_config = {
        .pin_bit_mask = 1ULL << PUMP_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&button_config));
}

void water_pump_set(bool enabled){
    if(enabled == pump_enabled){
        return;
    }

    pump_enabled = enabled;
    ESP_ERROR_CHECK(gpio_set_level(PUMP_GPIO, enabled ? 1 : 0));
    ESP_LOGI(TAG, "PUMP %s", enabled ? "ON" : "OFF");
}

bool water_pump_button_raw_pressed(){
    return gpio_get_level(PUMP_BUTTON_GPIO) == 0;
}
