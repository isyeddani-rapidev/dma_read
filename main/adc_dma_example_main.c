/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/adc.h"

#define TIMES 256
#define SOC_ADC_DIGI_MAX_BITWIDTH (12)

#define ADC_RESULT_BYTE 4
#define ADC_CONV_LIMIT_EN 1
#define ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define ADC_CONV_MODE ADC_CONV_SINGLE_UNIT_1

static uint16_t adc1_chan_mask = BIT(3);

static void continuous_adc_init(uint16_t adc1_chan_mask)
{
    adc_digi_init_config_t adc_dma_config = {
        .max_store_buf_size = 1024,
        .conv_num_each_intr = 10,
        .adc1_chan_mask = adc1_chan_mask,
        .adc2_chan_mask = 0,
    };
    printf("Mask: %u",adc_dma_config.adc1_chan_mask);
    ESP_ERROR_CHECK(adc_digi_initialize(&adc_dma_config));

    adc_digi_configuration_t dig_cfg = {
        .conv_limit_en = 1,
        .conv_limit_num = 1,
        .sample_freq_hz = 1000,
        .conv_mode = ADC_CONV_MODE,
        .format = ADC_OUTPUT_TYPE,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = 1;
    for (int i = 0; i < 1; i++)
    {
        adc_pattern[i].atten = ADC_ATTEN_DB_11;
        adc_pattern[i].channel = ADC1_CHANNEL_3;
        adc_pattern[i].unit = 0;
        adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_digi_controller_configure(&dig_cfg));
}

static bool check_valid_data(const adc_digi_output_data_t *data)
{
    int unit = data->type2.unit;
    if (unit >= ADC_UNIT_2)
    {
        return false;
    }
    if (data->type2.channel >= SOC_ADC_CHANNEL_NUM(unit))
    {
        return false;
    }
    return true;
}

void app_main(void)
{
    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[TIMES] = {0};
    continuous_adc_init(adc1_chan_mask);
    adc_digi_start();
    // vTaskDelay(1000);
    ret = adc_digi_read_bytes(result, TIMES, &ret_num, ADC_MAX_DELAY);
    printf("Task: %x, Read: %u\n", ret, ret_num);
    for (int i = 0; i < ret_num; i++)
    {
        adc_digi_output_data_t *p = (void *)&result[i];
        if (check_valid_data(p))
        {
            printf("%d: %d\n", i, p->type2.data);
        }
        else
        {
            printf("%d: \n", i);
        }
    }
    adc_digi_stop();
    ret = adc_digi_deinitialize();
    assert(ret == ESP_OK);
}