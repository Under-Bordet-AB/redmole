#include "gui_control_internal.h"

void gui_control_build_model(gui_control_t *control, gui_view_model_t *model)
{
    if ((control == NULL) || (model == NULL)) {
        return;
    }

    model->active_panel = control->active_panel;
    model->sensor = control->sensor;
    gui_control_clear_energy_plan(&model->energy_plan);
    model->wifi = control->wifi;
}