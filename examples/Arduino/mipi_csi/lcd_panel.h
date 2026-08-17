#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  lcd_panel_type_jd9365,
  lcd_panel_type_hx8394,
  lcd_panel_type_ili9881c,
} lcd_panel_type_t;

typedef struct {
  uint32_t width;
  uint32_t height;
} lcd_panel_info_t;

#ifndef LCD_PANEL_TYPE
#define LCD_PANEL_TYPE lcd_panel_type_jd9365
#endif

esp_err_t lcd_panel_init(esp_lcd_panel_handle_t *ret_panel, lcd_panel_info_t *panel_info);
esp_err_t lcd_panel_create(esp_lcd_panel_handle_t *ret_panel, lcd_panel_info_t *panel_info);
esp_err_t lcd_panel_get_info(lcd_panel_info_t *panel_info);

#ifdef __cplusplus
}
#endif
