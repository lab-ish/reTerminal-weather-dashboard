// battery 実装（M6）: GPIO21=H で分圧を有効化 → GPIO1(ADC1_CH0) を複数回平均 →
// esp_adc_cal 校正 → ×2.0 → LiPo曲線 → % / 5段階。
#include "battery.h"

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"

static const char *TAG = "battery";

#define BATT_ADC_UNIT     ADC_UNIT_1
#define BATT_ADC_CHANNEL  ADC_CHANNEL_0   // GPIO1 = ADC1_CH0
#define BATT_SAMPLES      16

esp_err_t battery_measure(int *out_percent, int *out_bucket)
{
    // 分圧イネーブル
    gpio_config_t en = {
        .pin_bit_mask = (1ULL << PIN_BATT_EN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&en);
    gpio_set_level(PIN_BATT_EN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    adc_oneshot_unit_handle_t adc = NULL;
    adc_oneshot_unit_init_cfg_t ucfg = { .unit_id = BATT_ADC_UNIT };
    esp_err_t err = adc_oneshot_new_unit(&ucfg, &adc);
    if (err != ESP_OK) { gpio_set_level(PIN_BATT_EN, 0); return err; }

    adc_oneshot_chan_cfg_t ccfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_oneshot_config_channel(adc, BATT_ADC_CHANNEL, &ccfg);

    // 校正（esp32s3: curve fitting）
    adc_cali_handle_t cali = NULL;
    adc_cali_curve_fitting_config_t cal_cfg = {
        .unit_id = BATT_ADC_UNIT,
        .chan = BATT_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    bool have_cali = (adc_cali_create_scheme_curve_fitting(&cal_cfg, &cali) == ESP_OK);

    long sum_mv = 0;
    int n = 0;
    for (int i = 0; i < BATT_SAMPLES; i++) {
        int raw = 0;
        if (adc_oneshot_read(adc, BATT_ADC_CHANNEL, &raw) != ESP_OK) continue;
        int mv = raw;
        if (have_cali) adc_cali_raw_to_voltage(cali, raw, &mv);
        sum_mv += mv;
        n++;
    }

    if (have_cali) adc_cali_delete_scheme_curve_fitting(cali);
    adc_oneshot_del_unit(adc);
    gpio_set_level(PIN_BATT_EN, 0);   // 測定終了で無効化（消費電力）

    if (n == 0) return ESP_FAIL;

    float adc_v = (sum_mv / (float)n) / 1000.0f;   // ADC 端子電圧[V]
    float batt_v = adc_v * BATT_DIVIDER;            // ×2.0
    int pct = batt_voltage_to_percent(batt_v);
    int bucket = batt_percent_to_bucket(pct, BATT_LEVELS);

    ESP_LOGI(TAG, "adc=%.3fV batt=%.3fV pct=%d bucket=%d", adc_v, batt_v, pct, bucket);
    if (out_percent) *out_percent = pct;
    if (out_bucket) *out_bucket = bucket;
    return ESP_OK;
}

int battery_read_percent(void)
{
    int pct = -1;
    if (battery_measure(&pct, NULL) != ESP_OK) return -1;
    return pct;
}
