#include "oled_display.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "OLED_DISPLAY";

#define OLED_I2C_PORT I2C_NUM_0
#define OLED_SDA_GPIO 6
#define OLED_SCL_GPIO 7
#define OLED_I2C_FREQ_HZ 100000
#define OLED_I2C_ADDR 0x3C

i2c_master_bus_handle_t oled_i2c_bus_handle = NULL;
i2c_master_dev_handle_t oled_i2c_dev_handle = NULL;

esp_err_t oled_write_command(uint8_t command){
    uint8_t data[2] = {
        0x00,   // control byte
        command // ssd1306 command
    };

    return i2c_master_transmit(oled_i2c_dev_handle, data, sizeof(data), 100);
}

void oled_display_init(){
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = OLED_I2C_PORT,
        .scl_io_num = OLED_SCL_GPIO,
        .sda_io_num = OLED_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &oled_i2c_bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = OLED_I2C_ADDR,
        .scl_speed_hz = OLED_I2C_FREQ_HZ
    };

    i2c_master_bus_add_device(oled_i2c_bus_handle, &dev_config, &oled_i2c_dev_handle);

    // esp_err_t res = i2c_master_probe(oled_i2c_bus_handle, OLED_I2C_ADDR, 100);
    // if(res == ESP_OK) ESP_LOGI(TAG, "Device address is correct.");

    ESP_ERROR_CHECK(oled_write_command(0x8D));
    ESP_ERROR_CHECK(oled_write_command(0x14));
    ESP_LOGI(TAG, "Charge pump enabled.");

    ESP_ERROR_CHECK(oled_write_command(0xAF));
    ESP_LOGI(TAG, "Display ON command sent.");

    ESP_ERROR_CHECK(oled_write_command(0xA5));
    ESP_LOGI(TAG, "Entire display ON command sent.");

    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_ERROR_CHECK(oled_write_command(0xA4));    
    ESP_LOGI(TAG, "Resumed to RAM content.");

    ESP_ERROR_CHECK(oled_write_command(0xAE));
    ESP_LOGI(TAG, "Display OFF command sent.");

}

