idf_component_register(SRCS "station_example_main.c"
                    INCLUDE_DIRS "."
                    REQUIRES freertos esp_event esp_netif esp_wifi nvs_flash lwip)
