#include "app_module.h"
#include <stdlib.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_err.h"

app_ctx_t *app_init(void)
{
    app_ctx_t *self = (app_ctx_t*)malloc(sizeof(app_ctx_t));
    if (!self) return NULL;
    memset(self, 0, sizeof(app_ctx_t));

    /* Initialize NVS */
    esp_err_t rv = nvs_flash_init();
    if (rv == ESP_ERR_NVS_NO_FREE_PAGES || rv == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        rv = nvs_flash_init();
    }
    ESP_ERROR_CHECK(rv);

    /* Init modules here */
    wifi_init(&self->wifi);

    return self;
}
/* We can save the stack space this takes and start all tasks in main
 * Then enter monitoring loop right in main too
void app_run(app_ctx_t *self);
*/

void app_deinit(app_ctx_t *self)
{
    /* TO DO
     * Implement tear-down
     */
}
