#include "soil_sensor.h"

adc_oneshot_unit_handle_t adc_handle;

void soil_sensor_init(){
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_DIGI_CLK_SRC_APB,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };

    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t channel_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &channel_cfg));
}

int soil_sensor_read_raw(){
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &raw)); 
    return raw;
}