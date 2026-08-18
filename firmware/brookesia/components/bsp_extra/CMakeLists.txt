set(SRCS "")
list(APPEND SRCS
    "src/bsp_board_extra.c"
    "src/bsp_board_ethernet.c"
)

set(INCLUDE_DIRS "")
list(APPEND INCLUDE_DIRS "include")

foreach(SRC_DIR IN LISTS SRC_DIRS)
    file(GLOB_RECURSE SRC ${SRC_DIR}/*.c)
    list(APPEND SRCS ${SRC})
    list(APPEND INCLUDE_DIRS ${SRC_DIR}/include)
endforeach()

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES driver esp_codec_dev esp_driver_gpio esp_driver_i2s esp_netif
    PRIV_REQUIRES esp32_p4_nano_wifi6_db esp_timer fatfs esp_psram esp_mm esp_driver_i2c esp_driver_ledc esp_eth esp_event ip101 lwip
)

target_compile_options(
    ${COMPONENT_LIB} PRIVATE
        -Wno-format
        -Wno-int-conversion
        -Wno-incompatible-pointer-types
        -Wunused-function
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-overflow
        -Wno-unused-but-set-variable
        -Wno-discarded-qualifiers)
