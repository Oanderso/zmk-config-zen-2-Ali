/* SPDX-License-Identifier: MIT */
#define DT_DRV_COMPAT zmk_input_processor_release_inertia

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <drivers/input_processor.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include "cursor_glide.h"
#include "../drivers/input/tps43_contact.h"

struct release_inertia_data {
    const struct device *dev;
    struct cursor_glide_state cursor;
    struct k_mutex lock;
    struct k_work_delayable work;
    int16_t pending_x, pending_y;
};

static void glide_work(struct k_work *work) {
    struct release_inertia_data *data = CONTAINER_OF(
        k_work_delayable_from_work(work), struct release_inertia_data, work);
    const struct cursor_glide_config *cfg = data->dev->config;
    k_mutex_lock(&data->lock, K_FOREVER);
    /* The driver updates this before queuing contact events. Do not send a
     * pending glide report while the input thread catches up to a new touch. */
    if (atomic_get(&tps43_contact_present)) {
        data->cursor.active = false;
        k_mutex_unlock(&data->lock);
        return;
    }
    int16_t dx, dy;
    if (cursor_glide_step(&data->cursor, cfg, &dx, &dy)) {
        zmk_hid_mouse_movement_set(dx, dy);
        zmk_endpoints_send_mouse_report();
        /* Do not leave relative movement in the shared HID report for a later
         * keyboard/button/scroll report to accidentally repeat. */
        zmk_hid_mouse_movement_set(0, 0);
        k_work_reschedule(&data->work, K_MSEC(cfg->interval_ms));
    }
    k_mutex_unlock(&data->lock);
}

static int handle_event(const struct device *dev, struct input_event *event,
                        uint32_t param1, uint32_t param2,
                        struct zmk_input_processor_state *state) {
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);
    ARG_UNUSED(state);
    struct release_inertia_data *data = dev->data;
    const struct cursor_glide_config *cfg = dev->config;
    int result = ZMK_INPUT_PROC_CONTINUE;
    k_mutex_lock(&data->lock, K_FOREVER);
    if (event->type == INPUT_EV_ABS && event->code == INPUT_ABS_MISC) {
        cursor_glide_contact(&data->cursor, cfg, event->value, k_uptime_get_32());
        if (data->cursor.active) {
            k_work_reschedule(&data->work, K_MSEC(cfg->interval_ms));
        } else {
            k_work_cancel_delayable(&data->work);
        }
        /* Contact metadata must not become a mouse/button report. */
        result = ZMK_INPUT_PROC_STOP;
    } else if ((event->type == INPUT_EV_KEY && event->value) ||
               (event->type == INPUT_EV_REL &&
                (event->code == INPUT_REL_WHEEL || event->code == INPUT_REL_HWHEEL))) {
        cursor_glide_block(&data->cursor);
        k_work_cancel_delayable(&data->work);
        data->pending_x = data->pending_y = 0;
    } else if (event->type == INPUT_EV_REL &&
               (event->code == INPUT_REL_X || event->code == INPUT_REL_Y)) {
        if (event->code == INPUT_REL_X) {
            data->pending_x = event->value;
        } else {
            data->pending_y = event->value;
        }
        if (event->sync) {
            cursor_glide_sample(&data->cursor, cfg, data->pending_x, data->pending_y,
                                k_uptime_get_32());
            data->pending_x = data->pending_y = 0;
        }
        /* Manual X/Y always pass through unchanged. Never arm a timer here. */
    }
    k_mutex_unlock(&data->lock);
    return result;
}

static int release_inertia_init(const struct device *dev) {
    struct release_inertia_data *data = dev->data;
    data->dev = dev;
    data->cursor.fingers = 255;
    data->cursor.blocked = true;
    k_mutex_init(&data->lock);
    k_work_init_delayable(&data->work, glide_work);
    return 0;
}

static const struct zmk_input_processor_driver_api release_inertia_api = {
    .handle_event = handle_event,
};

#define RELEASE_INERTIA_INST(n)                                                                    \
    static struct release_inertia_data release_data_##n;                                           \
    static const struct cursor_glide_config release_cfg_##n = {                                    \
        .interval_ms = DT_INST_PROP(n, report_interval_ms),                                        \
        .release_window_ms = DT_INST_PROP(n, release_window_ms),                                   \
        .start_threshold = DT_INST_PROP(n, start_threshold),                                       \
        .stop_threshold = DT_INST_PROP(n, stop_threshold),                                         \
        .retention_percent = DT_INST_PROP(n, retention_percent),                                   \
    };                                                                                            \
    DEVICE_DT_INST_DEFINE(n, release_inertia_init, NULL, &release_data_##n, &release_cfg_##n,         \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &release_inertia_api);

DT_INST_FOREACH_STATUS_OKAY(RELEASE_INERTIA_INST)
