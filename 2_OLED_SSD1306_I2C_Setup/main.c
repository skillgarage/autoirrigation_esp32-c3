#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "soil_sensor.h"
#include "oled_display.h"

static const char *TAG = "AUTO_IRRIGATION";

#define SOIL_WET 2200

static void soil_sensor_task(void *arg){
    while(1){
        int raw_value = soil_sensor_read_raw();
        ESP_LOGI(TAG, "Soil raw = %d", raw_value);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(){
    vTaskDelay(pdMS_TO_TICKS(5000));
    soil_sensor_init();
    oled_display_init();

    xTaskCreate(soil_sensor_task, "soil_sensor", 4096, NULL, 5, NULL);
}
