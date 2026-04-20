#include "app_module.h"
#include <stdlib.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "APP";

app_ctx_t *app_init(void)
{
    app_ctx_t *self = (app_ctx_t *)malloc(sizeof(app_ctx_t));
    if (!self) return NULL;
    memset(self, 0, sizeof(app_ctx_t));

    esp_err_t rv = nvs_flash_init();
    if (rv == ESP_ERR_NVS_NO_FREE_PAGES || rv == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS partition truncated — erasing and reinitialising");
        ESP_ERROR_CHECK(nvs_flash_erase());
        rv = nvs_flash_init();
    }
    ESP_ERROR_CHECK(rv);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Module init */

    self->nac = nac_init();
    if (!self->nac)
    {
        ESP_LOGE(TAG, "nac_init failed");
        free(self);
        return NULL;
    }

    ESP_LOGI(TAG, "Initialised");
    return self;
}

void app_deinit(app_ctx_t *self)
{
    if (!self) return;
    nac_dispose(self->nac);
    self->nac = NULL;
    /* TODO: esp_event_loop_delete_default(), esp_netif_deinit() when teardown is needed */
    free(self);
}
