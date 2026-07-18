// framebuffer 実装（M5）: PSRAM に 4bpp バッファを確保する。
#include "framebuffer.h"

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "app_config.h"

static const char *TAG = "framebuffer";
static uint8_t *s_fb;

esp_err_t framebuffer_alloc(void)
{
    if (s_fb) return ESP_OK;
    s_fb = heap_caps_malloc(EPD_FB_BYTES, MALLOC_CAP_SPIRAM);
    if (!s_fb) {
        ESP_LOGE(TAG, "PSRAM alloc %d B failed", EPD_FB_BYTES);
        return ESP_ERR_NO_MEM;
    }
    fb_fill(s_fb, COLOR_WHITE);
    ESP_LOGI(TAG, "framebuffer %d B on PSRAM", EPD_FB_BYTES);
    return ESP_OK;
}

uint8_t *framebuffer_get(void) { return s_fb; }

void framebuffer_free(void)
{
    if (s_fb) { heap_caps_free(s_fb); s_fb = NULL; }
}
