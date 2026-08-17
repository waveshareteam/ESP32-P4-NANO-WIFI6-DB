#pragma once

#include "../../../esp_panel_drivers_conf.h"
#include <esp_display_panel.hpp>

namespace mipi_dsi {

class LCD_JD9365_LOCAL final : public esp_panel::drivers::LCD {
public:
    static constexpr esp_panel::drivers::LCD::BasicAttributes BASIC_ATTRIBUTES_DEFAULT = {
        .name = "JD9365",
    };

    LCD_JD9365_LOCAL(esp_panel::drivers::Bus *bus, int width, int height, int color_bits, int rst_io):
        esp_panel::drivers::LCD(BASIC_ATTRIBUTES_DEFAULT, bus, width, height, color_bits, rst_io)
    {
    }

    LCD_JD9365_LOCAL(esp_panel::drivers::Bus *bus, const esp_panel::drivers::LCD::Config &config):
        esp_panel::drivers::LCD(BASIC_ATTRIBUTES_DEFAULT, bus, config)
    {
    }

    ~LCD_JD9365_LOCAL() override;

    bool init() override;

private:
    static const esp_panel::drivers::LCD::BasicBusSpecificationMap _bus_specifications;
};

}  // namespace mipi_dsi
