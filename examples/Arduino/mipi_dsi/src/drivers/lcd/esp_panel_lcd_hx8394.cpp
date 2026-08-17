#include "../../../esp_panel_drivers_conf.h"

#if ESP_PANEL_DRIVERS_LCD_ENABLE_HX8394_LOCAL

#include "esp_panel_lcd_hx8394.hpp"

#include <drivers/lcd/port/esp_panel_lcd_vendor_types.h>

#include "port/esp_lcd_hx8394.h"

#include <limits>
#include <stdint.h>
#include <variant>

namespace mipi_dsi {

using esp_panel::drivers::BusDSI;
using esp_panel::drivers::LCD;

const LCD::BasicBusSpecificationMap LCD_HX8394::_bus_specifications = {
    {
        ESP_PANEL_BUS_TYPE_MIPI_DSI, LCD::BasicBusSpecification{
            .color_bits = (1U << LCD::BasicBusSpecification::COLOR_BITS_RGB565_16) |
                          (1U << LCD::BasicBusSpecification::COLOR_BITS_RGB666_18) |
                          (1U << LCD::BasicBusSpecification::COLOR_BITS_RGB888_24),
            .functions = (1U << LCD::BasicBusSpecification::FUNC_INVERT_COLOR) |
                         (1U << LCD::BasicBusSpecification::FUNC_DISPLAY_ON_OFF),
        },
    },
};

LCD_HX8394::~LCD_HX8394()
{
    del();
}

bool LCD_HX8394::init()
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

    esp_panel_lcd_vendor_config_t hx8394_config = *vendor_config;
    hx8394_config.mipi_config.dsi_bus = dsi_bus->getHostHandle();
    hx8394_config.mipi_config.dpi_config = dpi_config;
    hx8394_config.mipi_config.lane_num = 2;

    auto panel_config = *device_config;
    panel_config.vendor_config = &hx8394_config;
    if (esp_lcd_new_panel_hx8394(
            getBus()->getControlPanelHandle(), &panel_config, &refresh_panel
        ) != ESP_OK) {
        return false;
    }

    setState(State::INIT);
    return true;
}

}  // namespace mipi_dsi

#endif // ESP_PANEL_DRIVERS_LCD_ENABLE_HX8394_LOCAL
