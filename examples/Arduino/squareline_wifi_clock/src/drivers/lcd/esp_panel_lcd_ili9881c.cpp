#include "../../../esp_panel_drivers_conf.h"

#if ESP_PANEL_DRIVERS_LCD_ENABLE_ILI9881C_LOCAL

#include "esp_panel_lcd_ili9881c.hpp"

#include <drivers/lcd/port/esp_panel_lcd_vendor_types.h>

#include "port/esp_lcd_ili9881c.h"

#include <limits>
#include <stdint.h>
#include <variant>

namespace squareline_wifi_clock {

using esp_panel::drivers::BusDSI;
using esp_panel::drivers::LCD;

const LCD::BasicBusSpecificationMap LCD_ILI9881C::_bus_specifications = {
    {
        ESP_PANEL_BUS_TYPE_MIPI_DSI, LCD::BasicBusSpecification{
            .color_bits = (1U << LCD::BasicBusSpecification::COLOR_BITS_RGB565_16) |
                          (1U << LCD::BasicBusSpecification::COLOR_BITS_RGB666_18) |
                          (1U << LCD::BasicBusSpecification::COLOR_BITS_RGB888_24),
            .functions = (1U << LCD::BasicBusSpecification::FUNC_INVERT_COLOR) |
                         (1U << LCD::BasicBusSpecification::FUNC_MIRROR_X) |
                         (1U << LCD::BasicBusSpecification::FUNC_MIRROR_Y) |
                         (1U << LCD::BasicBusSpecification::FUNC_DISPLAY_ON_OFF),
        },
    },
};

LCD_ILI9881C::~LCD_ILI9881C()
{
    del();
}

bool LCD_ILI9881C::init()
{
    if (isOverState(State::INIT)) {
        return false;
    }
    if (!processDeviceOnInit(_bus_specifications)) {
        return false;
    }
    if (getBus()->getBasicAttributes().type != ESP_PANEL_BUS_TYPE_MIPI_DSI) {
        return false;
    }

    auto *dsi_bus = static_cast<BusDSI *>(getBus());
    const auto *dpi_config = std::get_if<BusDSI::RefreshPanelFullConfig>(
        &dsi_bus->getConfig().refresh_panel
    );
    const auto *device_config = getConfig().getDeviceFullConfig();
    const auto *vendor_config = getConfig().getVendorFullConfig();
    if ((dpi_config == nullptr) || (device_config == nullptr) || (vendor_config == nullptr)) {
        return false;
    }
    if (vendor_config->init_cmds_size > std::numeric_limits<uint16_t>::max()) {
        return false;
    }

    esp_panel_lcd_vendor_config_t ili9881c_config = *vendor_config;
    ili9881c_config.mipi_config.dsi_bus = dsi_bus->getHostHandle();
    ili9881c_config.mipi_config.dpi_config = dpi_config;
    ili9881c_config.mipi_config.lane_num = 2;

    auto panel_config = *device_config;
    panel_config.vendor_config = &ili9881c_config;
    if (esp_lcd_new_panel_ili9881c(
            getBus()->getControlPanelHandle(), &panel_config, &refresh_panel
        ) != ESP_OK) {
        return false;
    }

    setState(State::INIT);
    return true;
}

}  // namespace squareline_wifi_clock

#endif // ESP_PANEL_DRIVERS_LCD_ENABLE_ILI9881C_LOCAL
