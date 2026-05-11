# Phase 2B — adds CH422G driver alongside i2c_bus.

idf_component_register(
    SRCS
        "main.c"
        "i2c_bus.c"
        "ch422g.c"
    INCLUDE_DIRS "."
    REQUIRES
        # core IDF
        esp_system
        esp_common
        log
        freertos
        spi_flash

        # peripherals
        driver
)
