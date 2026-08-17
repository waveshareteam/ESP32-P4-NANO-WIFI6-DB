#pragma once

#include "../../../esp_panel_drivers_conf.h"
#include <esp_display_panel.hpp>

namespace squareline_wifi_clock {

class LCD_HX8394 final : public esp_panel::drivers::LCD {
public:
    static constexpr esp_panel::drivers::LCD::BasicAttributes BASIC_ATTRIBUTES_DEFAULT = {
        .name = "HX8394",
    };

    LCD_HX8394(esp_panel::drivers::Bus *bus, int width, int height, int color_bits, int rst_io):
        esp_panel::drivers::LCD(BASIC_ATTRIBUTES_DEFAULT, bus, width, height, color_bits, rst_io)
    {
    }

    LCD_HX8394(esp_panel::drivers::Bus *bus, const esp_panel::drivers::LCD::Config &config):
        esp_panel::drivers::LCD(BASIC_ATTRIBUTES_DEFAULT, bus, config)
    {
    }

    ~LCD_HX8394() override;

    bool init() override;

private:
    static const esp_panel::drivers::LCD::BasicBusSpecificationMap _bus_specifications;
};

}  // namespace squareline_wifi_clock
