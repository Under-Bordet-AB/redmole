#include "task_scheduler.h"
#include "esp_log.h"

static task_scheduler_t g_task_scheduler;

esp_err_t task_scheduler_init(void)
{
    g_task_scheduler.head = NULL;
    g_task_scheduler.count = 0;
    g_task_scheduler.tag = "SCHEDULER";
    ESP_LOGI(g_task_scheduler.tag, "Initialized");
    return 0;
}

int8_t task_scheduler_add(task_node_t *node, uint32_t delay_ms)
{
    if (!node || !node->work)
    {
        ESP_LOGE(g_task_scheduler.tag, "Invalid task node");
        return -1;
    }

    if (node->active)
    {
        ESP_LOGW(g_task_scheduler.tag, "Task already active");
        return -1;
    }

    /* Calculate target tick: Current + Offset */
    node->run_at_tick = xTaskGetTickCount() + pdMS_TO_TICKS(delay_ms);

    /* Prepend to list */
    node->next = g_task_scheduler.head;
    g_task_scheduler.head = node;
    node->active = 1;
    g_task_scheduler.count++;

    return 0;
}

int8_t task_scheduler_remove(task_node_t *node)
{
    if (!node)
    {
        return -1;
    }

    task_node_t *curr = g_task_scheduler.head;
    task_node_t *prev = NULL;

    while (curr)
    {
        if (curr == node)
        {
            if (prev)
            {
                prev->next = curr->next;
            }
            else
            {
                g_task_scheduler.head = curr->next;
            }

            node->next = NULL;
            node->active = 0;
            g_task_scheduler.count--;
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    return -1;
}

int8_t task_scheduler_work(void)
{
    task_node_t *node = g_task_scheduler.head;
    TickType_t now = xTaskGetTickCount();

    while (node)
    {
        task_node_t *next = node->next;

        if (node->active && node->work)
        {

            /* Treats the timer like a circular clock. By subtracting and checking
             * if the result is positive, we know the deadline has passed even if
             * the 32-bit system timer just reset (rolled over) back to zero.
             */
            if ((int32_t)(now - node->run_at_tick) >= 0)
            {
                task_status_t status = node->work(node);

                if (status != TASK_RUN_AGAIN)
                {
                    if (status == TASK_ERROR)
                    {
                        ESP_LOGE(g_task_scheduler.tag, "Task error removal");
                    }

                    if (node->active)
                    {
                        task_scheduler_remove(node);
                    }
                }
            }
        }
        node = next;
    }

    return 0;
}
