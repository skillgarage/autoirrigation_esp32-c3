#pragma once

#include "esp_adc/adc_oneshot.h"

void soil_sensor_init();
int soil_sensor_read_raw();
