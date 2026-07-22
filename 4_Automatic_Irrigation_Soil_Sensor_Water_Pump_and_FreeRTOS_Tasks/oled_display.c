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

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES 8

i2c_master_bus_handle_t oled_i2c_bus_handle = NULL;
i2c_master_dev_handle_t oled_i2c_dev_handle = NULL;

void oled_set_cursor(uint8_t page, uint8_t column);
esp_err_t oled_write_data(const uint8_t *data, size_t len);

// Limit: 2200
// Soil: 1870

// ----- Font size 5*7 ----------------------

const uint8_t FONT_SPACE[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t FONT_COLON[5] = {0x00, 0x36, 0x36, 0x00, 0x00};

static const uint8_t FONT_DIGITS[10][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
};

static const uint8_t FONT_I[5] = {0x00, 0x41, 0x7F, 0x41, 0x00};
static const uint8_t FONT_L[5] = {0x7F, 0x40, 0x40, 0x40, 0x40};
static const uint8_t FONT_M[5] = {0x7F, 0x02, 0x0C, 0x02, 0x7F};
static const uint8_t FONT_O[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
static const uint8_t FONT_S[5] = {0x46, 0x49, 0x49, 0x49, 0x31};
static const uint8_t FONT_T[5] = {0x01, 0x01, 0x7F, 0x01, 0x01};

static const uint8_t *oled_get_char_bitmap(char c) // 7 != '7'; '0' - 48, '9' - 57
{
    if (c >= '0' && c <= '9') {
        return FONT_DIGITS[c - '0'];
    }

    switch (c) {
        case 'I': return FONT_I;
        case 'L': return FONT_L;
        case 'M': return FONT_M;
        case 'O': return FONT_O;
        case 'S': return FONT_S;
        case 'T': return FONT_T;
        case ':': return FONT_COLON;
        case ' ': return FONT_SPACE;
        default:  return FONT_SPACE;
    }
}

void oled_print_text(uint8_t page, uint8_t column, const char *text){ // 'Limit: 2200\0'
    oled_set_cursor(page, column);

    while(*text){
        const uint8_t *bitmap = oled_get_char_bitmap(*text);
        ESP_ERROR_CHECK(oled_write_data(bitmap, 5));
        uint8_t space = 0x00;
        ESP_ERROR_CHECK(oled_write_data(&space, 1));

        text++;
    }
}

void oled_display_clear(){
    uint8_t empty[OLED_WIDTH];
    memset(empty, 0x00, sizeof(empty));

    for(uint8_t page = 0; page < OLED_PAGES; page++){
        oled_set_cursor(page, 0);
        ESP_ERROR_CHECK(oled_write_data(empty, OLED_WIDTH));
    }
}

void oled_display_show_values(int soil_raw, int soil_limit){
    char line1[20];
    char line2[20];

    snprintf(line1, sizeof(line1), "SOIL: %d", soil_raw);
    snprintf(line2, sizeof(line2), "LIMIT: %d", soil_limit);

    oled_display_clear();

    oled_print_text(1, 0, line1);
    oled_print_text(3, 0, line2);
}

esp_err_t oled_write_data(const uint8_t *data, size_t len){
    uint8_t buffer[1 + OLED_WIDTH];

    if(len > OLED_WIDTH){
        len = OLED_WIDTH;
    }

    buffer[0] = 0x40;
    memcpy(&buffer[1], data, len);
    return i2c_master_transmit(oled_i2c_dev_handle, buffer, len + 1, 100);
}

esp_err_t oled_write_command(uint8_t command){
    uint8_t data[2] = {
        0x00,   // control byte
        command // ssd1306 command
    };

    return i2c_master_transmit(oled_i2c_dev_handle, data, sizeof(data), 100);
}

void oled_set_cursor(uint8_t page, uint8_t column){
    if(page > 7) page = 7;
    if(column > 127) column = 127;

    ESP_ERROR_CHECK(oled_write_command(0xB0 | page));
    ESP_ERROR_CHECK(oled_write_command(0x00 | (column & 0x0F))); // 1101 lower part
    ESP_ERROR_CHECK(oled_write_command(0x10 | (column >> 4))); // 0010 higher part
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

    ESP_ERROR_CHECK(oled_write_command(0xAE)); // Display OFF

    ESP_ERROR_CHECK(oled_write_command(0x20)); // Set Memory Addressing Mode
    ESP_ERROR_CHECK(oled_write_command(0x02)); // Page Addressing Mode

    ESP_ERROR_CHECK(oled_write_command(0xA8)); // Set Multiplex Ratio
    ESP_ERROR_CHECK(oled_write_command(0x3F)); // 64 MUX

    ESP_ERROR_CHECK(oled_write_command(0xD3)); // Set Display Offset
    ESP_ERROR_CHECK(oled_write_command(0x00)); // No offset

    ESP_ERROR_CHECK(oled_write_command(0x40)); // Set Display Start Line = 0

    ESP_ERROR_CHECK(oled_write_command(0xA1)); // Segment re-map
    ESP_ERROR_CHECK(oled_write_command(0xC8)); // COM output scan direction remapped

    ESP_ERROR_CHECK(oled_write_command(0xDA)); // Set COM Pins Hardware Configuration
    ESP_ERROR_CHECK(oled_write_command(0x12)); // Alternative COM pin config, 128x64

    ESP_ERROR_CHECK(oled_write_command(0x81)); // Set Contrast Control
    ESP_ERROR_CHECK(oled_write_command(0x7F)); // Default reset contrast value

    ESP_ERROR_CHECK(oled_write_command(0xA4)); // Display follows RAM content
    ESP_ERROR_CHECK(oled_write_command(0xA6)); // Normal display

    ESP_ERROR_CHECK(oled_write_command(0xD5)); // Clock divide / oscillator
    ESP_ERROR_CHECK(oled_write_command(0x80)); // Default

    ESP_ERROR_CHECK(oled_write_command(0x8D)); // Charge Pump Setting
    ESP_ERROR_CHECK(oled_write_command(0x14)); // Enable Charge Pump

    oled_display_clear();                      // Clear RAM while display is still OFF

    ESP_ERROR_CHECK(oled_write_command(0xAF)); // Display ON

    ESP_LOGI(TAG, "OLED initialized and cleared.");
}

