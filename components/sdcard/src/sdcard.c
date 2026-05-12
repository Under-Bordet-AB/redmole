#include "sdcard.h"
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/spi_common.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "io_extension.h"

#define TAG "SDCARD"

#define MOUNT_POINT "/sdcard"

static sdmmc_card_t *card = NULL;
static bool initialized = false;

#define PIN_SD_MOSI  11
#define PIN_SD_MISO  13
#define PIN_SD_CLK   12
#define PIN_SD_CS    GPIO_NUM_NC

esp_err_t sdcard_init(void)
{
    if (initialized) {
        return ESP_OK;
    }

    esp_err_t ret;
    ret = board_i2c_init();
    if (ret != ESP_OK){
        return ret;
    }
    IO_EXTENSION_Init();
    IO_EXTENSION_Output(4, 0);
    vTaskDelay(pdMS_TO_TICKS(50));


    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_SD_MOSI,
        .miso_io_num = PIN_SD_MISO,
        .sclk_io_num = PIN_SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed");
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_SD_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
    };


    ret = esp_vfs_fat_sdspi_mount(
        MOUNT_POINT,
        &host,
        &slot_config,
        &mount_config,
        &card
    );

    if (ret == ESP_OK) {
        initialized = true;
        ESP_LOGI(TAG, "SD card mounted");
    } else {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

void sdcard_dispose(void)
{
    if (!initialized) {
        return;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unmount failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SD card unmounted");
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_free(host.slot);

    card = NULL;
    initialized = false;
}

bool sdcard_is_initialized(void)
{
    return initialized;
}

esp_err_t sdcard_write_file(const char *path, const char *data)
{
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    fprintf(f, "%s", data);
    fclose(f);

    return ESP_OK;
}

esp_err_t sdcard_append_file(const char *path, const char *data)
{
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    FILE *f = fopen(path, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    fprintf(f, "%s", data);
    fclose(f);

    return ESP_OK;
}

esp_err_t sdcard_read_file(const char *path, char *out_buffer, size_t max_len)
{
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    size_t read_bytes = fread(out_buffer, 1, max_len - 1, f);
    out_buffer[read_bytes] = '\0';

    fclose(f);

    ESP_LOGI(TAG, "File read: %s (%zu bytes)", path, read_bytes);
    return ESP_OK;
}