/**
 * @file
 * @brief Cooperative task scheduler for FreeRTOS.
 *
 * Modules embed a task_node_t in their own struct and register it with the
 * scheduler. The scheduler calls each node's work function on or after its
 * scheduled tick. The work function returns TASK_RUN_AGAIN to reschedule
 * itself or TASK_DONE to be removed. Callers can adjust run_at_tick inside
 * the work function before returning TASK_RUN_AGAIN for dynamic delay control.
 */

#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include <stddef.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Return codes for scheduled work functions.
 */
typedef enum
{
    TASK_ERROR     = -1, /*!< Work encountered an error; node is removed from the scheduler. */
    TASK_DONE      =  0, /*!< Work is complete; node is removed from the scheduler. */
    TASK_RUN_AGAIN =  1, /*!< Work should be called again; node is kept in the scheduler. */
} task_status_t;

/**
 * @brief Recover a pointer to the enclosing struct from a pointer to a member.
 */
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

typedef struct task_node task_node_t;

/**
 * @brief Work callback invoked by the scheduler for each active node.
 *
 * Recover the enclosing module struct with container_of(node, my_type_t, task_node).
 */
typedef task_status_t (*task_scheduler_work_fn)(task_node_t *node);

/**
 * @brief Intrusive linked-list node representing one scheduled work item.
 *
 * Embed this in a module's context struct. The scheduler owns the list
 * linkage; the module owns the work function pointer and run time.
 */
struct task_node
{
    task_scheduler_work_fn work;        /*!< Work callback; must not be NULL when the node is active. */
    task_node_t           *next;        /*!< Next node in the scheduler list; managed by the scheduler. */
    uint32_t               run_at_tick; /*!< FreeRTOS tick at or after which work is invoked. */
    uint8_t                active;      /*!< Non-zero while the node is queued in the scheduler. */
};

/**
 * @brief Scheduler instance owning the active task list.
 */
typedef struct task_scheduler
{
    task_node_t *head;  /*!< Head of the intrusive singly-linked list of active nodes. */
    uint8_t      count; /*!< Number of nodes currently in the list. */
    const char  *tag;   /*!< Log tag string, not owned by this struct. */
} task_scheduler_t;

/**
 * @brief Initialise the task scheduler.
 *
 * Must be called once before task_scheduler_add(). The scheduler is driven
 * by calling task_scheduler_work() in a loop from the caller's context; no
 * separate FreeRTOS task is created. Not idempotent.
 *
 * @return ESP_OK on success, an esp_err_t error code on failure.
 */
esp_err_t task_scheduler_init(void);

/**
 * @brief Stop the scheduler task and release all resources.
 *
 * Removes all queued nodes without calling their work functions.
 *
 * @return 0 on success, -1 on failure.
 */
int8_t task_scheduler_deinit(void);

/**
 * @brief Add a task node to the scheduler, optionally after a delay.
 *
 * If the node is already queued it is not added a second time.
 *
 * @param node  Node to add; must not be NULL and must have a valid work pointer.
 * @param delay Ticks to wait before first invocation; 0 runs on the next tick.
 * @return 0 on success, -1 on failure.
 */
int8_t task_scheduler_add(task_node_t *node, uint32_t delay);

/**
 * @brief Remove a node from the scheduler without calling its work function.
 *
 * Safe to call from inside a work callback. No-op if the node is not queued.
 *
 * @param node Node to remove; must not be NULL.
 * @return 0 on success, -1 on failure.
 */
int8_t task_scheduler_remove(task_node_t *node);

/**
 * @brief Run one pass of the scheduler, invoking all nodes whose run time has elapsed.
 *
 * Nodes returning TASK_DONE or TASK_ERROR are removed; nodes returning
 * TASK_RUN_AGAIN are kept and re-evaluated on the next call. Intended to be
 * called from the scheduler FreeRTOS task in a tight loop.
 *
 * @return 0 if at least one node was executed, -1 if the list was empty.
 */
int8_t task_scheduler_work(void);

#endif /* __task_scheduler_h__ */
