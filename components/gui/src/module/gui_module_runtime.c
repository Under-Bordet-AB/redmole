#include "gui_module_internal.h"

static gui_module_runtime_t s_runtime;

gui_module_runtime_t *gui_module_get_singleton(void)
{
    return &s_runtime;
}

gui_module_runtime_t *gui_module_get_runtime(gui_ctx_t *self)
{
    if ((self == NULL) || (self->module_state == NULL)) {
        return NULL;
    }

    return (gui_module_runtime_t *)self->module_state;
}

void gui_module_apply_model(gui_module_runtime_t *runtime)
{
    gui_view_model_t model = { 0 };

    if (runtime == NULL) {
        return;
    }

    gui_control_build_model(&runtime->control, &model);
    gui_view_apply(&runtime->view, &model);
}