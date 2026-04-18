/**
 * Header file: task_scheduler.h
 * Description: Task scheduler module for FreeRTOS.
 * - User can add tasks to the scheduler and have them run at a later time or immediately.
 * - The task can being ran by the scheduler can itself decide if it should run again or not using enum values as return codes.
 *
 * - Becauase the task_node_t is baked into the modules struct, the user does not need to allocate memory for it.
 * - This also allows manipulating the task_node_t directly, for example:
 *  - setting the work function as long as it is a valid function pointer: task_status_t my_work_fnc(task_node_t *node)
 *  - dynamically deciding on delay based on the task's state
 *      ex: task_status_t my_work_fnc(task_node_t *node)
 *      {
 *          task_node_t *self = container_of(node, task_node_t, work);
 *          ...
 *          if (no_data)
 *          {
 *              self->node.run_at_tick = xTaskGetTickCount() + pdMS_TO_TICKS(50);
 *              return TASK_RUN_AGAIN;
 *          }
 *          return TASK_DONE;
 *      }
 **/

#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include <stddef.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


typedef enum
{
    TASK_ERROR      = -1,
    TASK_DONE       = 0,
    TASK_RUN_AGAIN  = 1,
} task_status_t;

/**
 * container_of macro - gets parent struct from member pointer
 */
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

typedef struct task_node task_node_t;

/**
 * Callback function that all modules will use to
 * call back to the scheduler
 **/
typedef task_status_t (*task_scheduler_work_fn)(task_node_t *node);

struct task_node
{
    task_scheduler_work_fn work;
    task_node_t *next;
    uint32_t run_at_tick;
    uint8_t active;
};

typedef struct task_scheduler
{
    task_node_t *head;
    uint8_t     count;
    const char  *tag;
} task_scheduler_t;

int8_t task_scheduler_init(void);
int8_t task_scheduler_deinit(void);

/* @brief Adds a task to the scheduler, with or without a delay
 * @param node The task node to add
 * @param delay The delay in ticks before the task is run, 0 for no delay
 * @return 0 on success, -1 on failure
 */
int8_t task_scheduler_add(task_node_t *node, uint32_t delay);

int8_t task_scheduler_remove(task_node_t *node);
int8_t task_scheduler_work(void);

#endif /* __task_scheduler_h__ */
