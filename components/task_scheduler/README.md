# Task Scheduler

A lightweight cooperative task scheduler for FreeRTOS. Modules embed a `task_node_t` in their own context struct and register it with the scheduler. The scheduler calls each node's work function on or after its scheduled tick. No separate FreeRTOS task is created; the caller drives execution by calling `task_scheduler_work()` in a loop.

## Design

The scheduler maintains an intrusive singly-linked list of `task_node_t` values. Because the node is embedded directly in the module struct, no separate heap allocation is needed per task, and the work callback can recover the full module context using `container_of`.

```c
// Recover the enclosing struct inside a work callback:
my_module_t *self = container_of(node, my_module_t, task_node);
```

### Work function return codes

| Value | Effect |
|---|---|
| `TASK_RUN_AGAIN` | Node stays in the scheduler; work is called again. |
| `TASK_DONE` | Node is removed from the scheduler. |
| `TASK_ERROR` | Node is removed from the scheduler (error path). |

The work function may adjust `node->run_at_tick` before returning `TASK_RUN_AGAIN` to implement dynamic delays.

## API

```c
esp_err_t task_scheduler_init(void);
int8_t    task_scheduler_deinit(void);

int8_t    task_scheduler_add(task_node_t *node, uint32_t delay);
int8_t    task_scheduler_remove(task_node_t *node);

int8_t    task_scheduler_work(void);
```

### Typical usage

```c
// 1. Init once at startup.
task_scheduler_init();

// 2. Add a node (runs after `delay` ticks; 0 = next tick).
task_scheduler_add(&my_module.task_node, pdMS_TO_TICKS(100));

// 3. Drive the scheduler from a FreeRTOS task loop.
while (1)
{
    task_scheduler_work();
    vTaskDelay(1);
}

// 4. Tear down.
task_scheduler_deinit();
```

`task_scheduler_deinit()` removes all queued nodes without invoking their work functions. `task_scheduler_add()` is a no-op if the node is already queued. `task_scheduler_remove()` is safe to call from inside a work callback.

## Structs

### `task_node_t`

| Field | Description |
|---|---|
| `work` | Work callback; must not be NULL while the node is active. |
| `next` | Intrusive list linkage; managed by the scheduler. |
| `run_at_tick` | FreeRTOS tick at or after which `work` is invoked. |
| `active` | Non-zero while the node is queued. |

### `task_scheduler_t`

| Field | Description |
|---|---|
| `head` | Head of the active node list. |
| `count` | Number of nodes currently queued. |
| `tag` | Log tag string (not owned). |

## Dependencies

- FreeRTOS (`freertos/task.h`) for tick count via `xTaskGetTickCount()`.
- ESP-IDF `esp_err.h` for the `esp_err_t` return type of `task_scheduler_init()`.
