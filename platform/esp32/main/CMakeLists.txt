# Phase 2A — main component with I2C bus support.
# Add SRCS / REQUIRES as more features land.

idf_component_register(
    SRCS
        "main.c"
        "i2c_bus.c"
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
