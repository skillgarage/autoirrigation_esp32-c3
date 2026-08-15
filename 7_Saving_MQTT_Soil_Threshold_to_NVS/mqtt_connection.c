#include "mqtt_connection.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "app_state.h"
#include "settings_storage.h"

static const char *TAG = "MQTT";

#define MQTT_BROKER_URI "mqtt://broker.emqx.io:1883"
#define MQTT_THRESHOLD_TOPIC "auto_irrigation/esp32c3/threshold/set"

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id){
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to MQTT broker.");

            esp_mqtt_client_subscribe(event->client, MQTT_THRESHOLD_TOPIC, 1);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Subscribed successfully, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            char threshold_text[16];
            if(event->data_len >= sizeof(threshold_text)){
                ESP_LOGE(TAG, "Received value is too long.");
                break;
            }

            memcpy(threshold_text, event->data, event->data_len);
            threshold_text[event->data_len] = '\0'; // 2300\0

            char *end_ptr;
            long new_threshold = strtol(threshold_text, &end_ptr, 10); // 2300\0

            if(*end_ptr != '\0' ||  new_threshold > 4095){
                ESP_LOGE(TAG, "Threshold out of range: %ld", new_threshold);
                break;
            }
            ESP_LOGI(TAG, "New threshold received: %ld", new_threshold);
            soil_threshold = (int)new_threshold;
            settings_threshold_save_threshold(soil_threshold);
            ESP_LOGI(TAG, "Threshold updated %d", soil_threshold);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected from MQTT broker.");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT connection error.");
            break;

        default:
            break;
    }
}

void mqtt_connection_start(void){
    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = MQTT_BROKER_URI
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_config);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_mqtt_client_start(client);
}
