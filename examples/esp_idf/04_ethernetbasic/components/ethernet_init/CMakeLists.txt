set(priv_requires esp_driver_gpio esp_eth)

if(DEFINED IDF_VERSION_MAJOR AND IDF_VERSION_MAJOR GREATER_EQUAL 6)
    list(APPEND priv_requires ip101)
endif()

idf_component_register(SRCS "ethernet_init.c"
                       PRIV_REQUIRES ${priv_requires}
                       INCLUDE_DIRS ".")
