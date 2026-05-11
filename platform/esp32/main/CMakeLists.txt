# Phase 2C — adds SD card driver alongside i2c_bus and ch422g.

idf_component_register(
    SRCS
        "main.c"
        "i2c_bus.c"
        "ch422g.c"
        "sd_card.c"
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

        # SD card stack (built-in components, NOT managed)
        fatfs
        sdmmc
        esp_driver_sdmmc
)
