#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "soil_sensor.h"
#include "oled_display.h"
#include "water_pump.h"
#include "wifi_connection.h"
#include "mqtt_connection.h"
#include "nvs_flash.h"
#include "settings_storage.h"

// MQTT - Message Queuing Telemetry Transport
// Address broker: broker.emqx.io
// Port MQTT: 1883
// Topic: auto_irrigation/esp32c3/threshold/set
// MQTTX Web - mqttx.app/web-client

// NVS - Non volatile storage

static const char *TAG = "AUTO_IRRIGATION";

// #define SOIL_WET 2200

#define PUMP_WORK_TIME_MS 15000
#define SOIL_CHECK_DELAY_MS 120000

static volatile int soil_raw_value = 0;
volatile int soil_threshold = 2200;

static void soil_sensor_task(void *arg){
    while(1){
        soil_raw_value = soil_sensor_read_raw();
        ESP_LOGI(TAG, "Soil raw = %d, Limit = %d", soil_raw_value, soil_threshold);
        oled_display_show_values(soil_raw_value, soil_threshold);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void pump_task(void *arg){
    bool pump_running = false;
    bool waiting = false;

    bool previous_button_state = false;

    TickType_t pump_started = 0;
    TickType_t waiting_started = 0;

    while(1){
        bool button_pressed = water_pump_button_raw_pressed();
        if(button_pressed != previous_button_state){
            vTaskDelay(pdMS_TO_TICKS(30));
            button_pressed = water_pump_button_raw_pressed();
        }

        bool new_button_press = button_pressed && !previous_button_state;
        previous_button_state = button_pressed;

        TickType_t now = xTaskGetTickCount();

        // Check if pump started, wait 15 sec
        if(pump_running){
            if((now - pump_started) >= pdMS_TO_TICKS(PUMP_WORK_TIME_MS)){
                water_pump_set(false);
                pump_running = false;
                waiting = true;
                waiting_started = now;
                ESP_LOGI(TAG, "Pump stopped, waiting 2 min.");
            }
        }
        // If pump stopped, check for start conditions
        else {
            if(new_button_press){ // if button pressed
                water_pump_set(true);
                pump_running = true;
                waiting = false;
                pump_started = now;
                ESP_LOGI(TAG, "Pump started manually.");
            }
            else if(waiting){ // if 2 min waiting over
                if((now - waiting_started) >= pdMS_TO_TICKS(SOIL_CHECK_DELAY_MS)){
                    waiting = false;
                    ESP_LOGI(TAG, "Waiting finished.");
                }
            }
            else if(soil_raw_value > soil_threshold){ // if no waiting, check sensor
                water_pump_set(true);
                pump_running = true;
                pump_started = now;
                ESP_LOGI(TAG, "Soil is dry, pump started.");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(){
    vTaskDelay(pdMS_TO_TICKS(2500));
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGI(TAG, "NVS initialized.");
    settings_storage_init();
    soil_threshold = settings_storage_load_threshold(soil_threshold);
    water_pump_init();
    soil_sensor_init();
    oled_display_init();
    vTaskDelay(pdMS_TO_TICKS(500));

    wifi_connection_init();
    mqtt_connection_start();

    xTaskCreate(soil_sensor_task, "soil_sensor", 4096, NULL, 5, NULL);
    xTaskCreate(pump_task, "pump_task", 4096, NULL, 5, NULL);
}
