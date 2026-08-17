#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_lcd_mipi_dsi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int cmd;
    const void *data;
    size_t data_bytes;
    unsigned int delay_ms;
} esp_panel_lcd_vendor_init_cmd_t;

typedef struct {
    int hor_res;
    int ver_res;
    const esp_panel_lcd_vendor_init_cmd_t *init_cmds;
    unsigned int init_cmds_size;
    struct {
        uint8_t lane_num;
        esp_lcd_dsi_bus_handle_t dsi_bus;
        const esp_lcd_dpi_panel_config_t *dpi_config;
    } mipi_config;
} esp_panel_lcd_vendor_config_t;

#ifdef __cplusplus
}
#endif
