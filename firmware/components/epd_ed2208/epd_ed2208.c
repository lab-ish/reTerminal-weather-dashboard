// epd_ed2208 ドライバ実装。Seeed_GxEPD2 GxEPD2_730c_GDEP073E01 の手順を ESP-IDF へ移植。
#include "epd_ed2208.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"

static const char *TAG = "epd_ed2208";

// Seeed_GxEPD2 準拠のタイミング（ms）。実機（M8）で調整。
#define EPD_POWER_ON_TIME_MS    200
#define EPD_POWER_OFF_TIME_MS   150
#define EPD_FULL_REFRESH_TIME_MS 30000   // 実測25-30s。余裕をみる
#define EPD_SPI_HZ              (10 * 1000 * 1000)  // 10MHz（保守的）
#define EPD_SPI_HOST            SPI2_HOST
#define EPD_CHUNK               4096      // データ転送の分割サイズ（内部RAMバウンス）

static spi_device_handle_t s_spi;
static uint8_t *s_bounce;                 // 内部DMA可能RAMのバウンスバッファ

// ---- 低レベル SPI/GPIO -----------------------------------------------------

static inline void cs(int level) { gpio_set_level(PIN_EPD_CS, level); }
static inline void dc(int level) { gpio_set_level(PIN_EPD_DC, level); }

static esp_err_t spi_tx(const uint8_t *data, size_t len)
{
    spi_transaction_t t = {0};
    t.length = len * 8;      // bits
    t.tx_buffer = data;
    return spi_device_polling_transmit(s_spi, &t);
}

// GxEPD2 _writeCommand: CS↓ DC↓ 1byte CS↑
static void epd_cmd(uint8_t c)
{
    dc(0); cs(0);
    spi_tx(&c, 1);
    cs(1);
}

// GxEPD2 _writeData: CS↓ DC↑ 1byte CS↑
static void epd_data(uint8_t d)
{
    dc(1); cs(0);
    spi_tx(&d, 1);
    cs(1);
}

// 大きなデータブロック: CS↓ DC↑ … 連続転送 … CS↑（バウンスバッファ経由でPSRAM対応）
static esp_err_t epd_data_block(const uint8_t *buf, size_t len)
{
    dc(1); cs(0);
    esp_err_t err = ESP_OK;
    for (size_t off = 0; off < len && err == ESP_OK; off += EPD_CHUNK) {
        size_t n = (len - off) < EPD_CHUNK ? (len - off) : EPD_CHUNK;
        memcpy(s_bounce, buf + off, n);   // PSRAM→内部RAM
        err = spi_tx(s_bounce, n);
    }
    cs(1);
    return err;
}

// busy はアクティブ Low。Low の間は待機。
static esp_err_t wait_busy(const char *who, uint32_t timeout_ms)
{
    const TickType_t start = xTaskGetTickCount();
    while (gpio_get_level(PIN_EPD_BUSY) == 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(timeout_ms)) {
            ESP_LOGE(TAG, "busy timeout: %s", who);
            return ESP_ERR_TIMEOUT;
        }
    }
    return ESP_OK;
}

static void power_on(void)
{
    epd_cmd(0x04);
    wait_busy("power_on", EPD_POWER_ON_TIME_MS + 200);
}

static void power_off(void)
{
    epd_cmd(0x02);
    epd_data(0x00);
    wait_busy("power_off", EPD_POWER_OFF_TIME_MS + 200);
}

// 全画面をパネルRAMウィンドウに設定（cmd 0x83, 9バイト）。
static void set_ram_area_full(void)
{
    const uint16_t x = 0, y = 0, w = EPD_WIDTH, h = EPD_HEIGHT;
    const uint16_t xe = x + w - 1;
    const uint16_t ye = y + h;  // controller issue（GxEPD2 コメント: one more）
    epd_cmd(0x83);
    epd_data(x >> 8);  epd_data(x & 0xFF);
    epd_data(xe >> 8); epd_data(xe & 0xFF);
    epd_data(y >> 8);  epd_data(y & 0xFF);
    epd_data(ye >> 8); epd_data(ye & 0xFF);
    epd_data(0x01);
}

// パネル初期化レジスタ列（GxEPD2 _InitDisplay 準拠）。
static void init_registers(void)
{
    epd_cmd(0xAA);  // CMDH
    epd_data(0x49); epd_data(0x55); epd_data(0x20);
    epd_data(0x08); epd_data(0x09); epd_data(0x18);
    epd_cmd(0x01);  epd_data(0x3F);                       // PWRR
    epd_cmd(0x00);  epd_data(0x5F); epd_data(0x69);       // PSR
    epd_cmd(0x03);  epd_data(0x00); epd_data(0x54);       // POFS
    epd_data(0x00); epd_data(0x44);
    epd_cmd(0x05);  epd_data(0x40); epd_data(0x1F);       // BTST1
    epd_data(0x1F); epd_data(0x2C);
    epd_cmd(0x06);  epd_data(0x6F); epd_data(0x1F);       // BTST2
    epd_data(0x17); epd_data(0x49);
    epd_cmd(0x08);  epd_data(0x6F); epd_data(0x1F);       // BTST3
    epd_data(0x1F); epd_data(0x22);
    epd_cmd(0x30);  epd_data(0x08);                       // PLL
    epd_cmd(0x50);  epd_data(0x3F);                       // CDI
    epd_cmd(0x60);  epd_data(0x02); epd_data(0x00);       // TCON
    epd_cmd(0x61);  epd_data(0x03); epd_data(0x20);       // TRES 800
    epd_data(0x01); epd_data(0xE0);                       //      480
    epd_cmd(0x84);  epd_data(0x01);                       // T_VDCS
    epd_cmd(0xE3);  epd_data(0x2F);                       // PWS
}

static void reset(void)
{
    gpio_set_level(PIN_EPD_RST, 1); vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(PIN_EPD_RST, 0); vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_EPD_RST, 1); vTaskDelay(pdMS_TO_TICKS(10));
}

// ---- 公開API ---------------------------------------------------------------

esp_err_t epd_init(void)
{
    // GPIO
    gpio_config_t out = {
        .pin_bit_mask = (1ULL << PIN_EPD_CS) | (1ULL << PIN_EPD_DC) |
                        (1ULL << PIN_EPD_RST),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&out);
    gpio_config_t in = {
        .pin_bit_mask = (1ULL << PIN_EPD_BUSY),
        .mode = GPIO_MODE_INPUT,
    };
    gpio_config(&in);
    cs(1);

    // バウンスバッファ（内部DMA可能RAM）
    if (!s_bounce) {
        s_bounce = heap_caps_malloc(EPD_CHUNK, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!s_bounce) return ESP_ERR_NO_MEM;
    }

    // SPI バス + デバイス（CS は手動制御のため spics=-1）
    spi_bus_config_t bus = {
        .sclk_io_num = PIN_EPD_SCK,
        .mosi_io_num = PIN_EPD_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EPD_CHUNK,
    };
    esp_err_t err = spi_bus_initialize(EPD_SPI_HOST, &bus, SPI_DMA_CH_AUTO);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    spi_device_interface_config_t dev = {
        .clock_speed_hz = EPD_SPI_HZ,
        .mode = 0,
        .spics_io_num = -1,   // 手動CS
        .queue_size = 1,
    };
    err = spi_bus_add_device(EPD_SPI_HOST, &dev, &s_spi);
    if (err != ESP_OK) return err;

    reset();
    init_registers();
    power_on();
    ESP_LOGI(TAG, "init done");
    return ESP_OK;
}

esp_err_t epd_write_framebuffer(const uint8_t *fb, size_t len)
{
    if (!fb) return ESP_ERR_INVALID_ARG;
    if (len != (size_t)EPD_FB_BYTES) {
        ESP_LOGE(TAG, "unexpected fb size %u (want %d)", (unsigned)len, EPD_FB_BYTES);
        return ESP_ERR_INVALID_SIZE;
    }
    set_ram_area_full();
    epd_cmd(0x10);                       // DTM: 以降ネイティブ4bppをそのまま流す
    return epd_data_block(fb, len);
}

esp_err_t epd_refresh(void)
{
    power_on();
    set_ram_area_full();
    epd_cmd(0x50); epd_data(0x3F);       // CDI border
    epd_cmd(0x12); epd_data(0x00);       // DRF display refresh
    vTaskDelay(pdMS_TO_TICKS(1));
    return wait_busy("refresh", EPD_FULL_REFRESH_TIME_MS);
}

esp_err_t epd_sleep(void)
{
    power_off();
    epd_cmd(0x07); epd_data(0xA5);       // deep sleep
    ESP_LOGI(TAG, "panel sleep");
    return ESP_OK;
}
