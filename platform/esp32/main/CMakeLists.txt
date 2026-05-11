# Phase 2D — adds OTA module alongside i2c_bus, ch422g, sd_card.

idf_component_register(
    SRCS
        "main.c"
        "i2c_bus.c"
        "ch422g.c"
        "sd_card.c"
        "ota.c"
    INCLUDE_DIRS "."
    REQUIRES
        # core IDF
        esp_system
        esp_common
        log
        freertos
        spi_flash
        esp_timer

        # peripherals
        driver

        # SD card stack
        fatfs
        sdmmc
        esp_driver_sdmmc

        # OTA
        app_update
        esp_partition
        mbedtls
)
